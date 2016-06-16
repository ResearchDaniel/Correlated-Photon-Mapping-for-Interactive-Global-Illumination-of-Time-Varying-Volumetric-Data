/* Copyright (c) 2012, 2014 University of Cape Town
 * Copyright (c) 2014 Bruce Merry
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file
 *
 * Utility functions that are private to the library.
 */

#include "clhpp11.h"

#include <clogs/visibility_push.h>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <locale>
#include <algorithm>
#include <cassert>
#include <clogs/visibility_pop.h>

#include <clogs/core.h>
#include "utils.h"
#include "cache.h"

namespace clogs
{
namespace detail
{
static std::map<std::string, Source> g_sources = std::map<std::string, Source>();

std::map<std::string, Source> &getSourceMap() { return g_sources; };


void Algorithm::doEventCallback(const cl::Event &event)
{
    if (eventCallback != NULL)
        (*eventCallback)(event(), eventCallbackUserData);
}

void Algorithm::setEventCallback(
    void (CL_CALLBACK *callback)(cl_event, void *),
    void *userData,
    void (CL_CALLBACK *free)(void *))
{
    if (eventCallbackFree != NULL)
        eventCallbackFree(eventCallbackUserData);
    eventCallback = callback;
    eventCallbackUserData = userData;
    eventCallbackFree = free;
}

Algorithm::Algorithm()
    : eventCallback(NULL), eventCallbackFree(NULL), eventCallbackUserData(NULL)
{
}

Algorithm::~Algorithm()
{
    if (eventCallbackFree != NULL)
        eventCallbackFree(eventCallbackUserData);
}

bool deviceHasExtension(const cl::Device &device, const std::string &extension)
{
    std::string extensions = device.getInfo<CL_DEVICE_EXTENSIONS>();
    std::string::size_type pos = extensions.find(extension);
    bool found = false;
    while (pos != std::string::npos)
    {
        if ((pos == 0 || extensions[pos - 1] == ' ')
            && (pos + extension.size() == extensions.size() || extensions[pos + extension.size()] == ' '))
        {
            found = true;
            break;
        }
        pos = extensions.find(extension, pos + 1);
    }
    return found;
}

unsigned int getWarpSizeMem(const cl::Device &device)
{
    /* According to an AMD engineer, AMD GPU wavefronts do not guarantee the
     * synchronization semantics implied by this function, so we do not
     * try to detect them.
     */
    if (deviceHasExtension(device, "cl_nv_device_attribute_query"))
        return device.getInfo<CL_DEVICE_WARP_SIZE_NV>();
    else
        return 1U;
}

unsigned int getWarpSizeSchedule(const cl::Device &device)
{
    if (deviceHasExtension(device, "cl_nv_device_attribute_query"))
        return device.getInfo<CL_DEVICE_WARP_SIZE_NV>();
    else
    {
        cl::Platform platform(device.getInfo<CL_DEVICE_PLATFORM>());
        if (platform.getInfo<CL_PLATFORM_NAME>() == "AMD Accelerated Parallel Processing")
        {
            if (device.getInfo<CL_DEVICE_TYPE>() & CL_DEVICE_TYPE_GPU)
                return 64U; // true for many AMD GPUs, not all
            else
                return 1U;  // might eventually need to change if autovectorization is done
        }
    }
    return 1U;
}

cl::Context contextForDevice(const cl::Device &device)
{
    cl_context_properties props[3] =
    {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties) device.getInfo<CL_DEVICE_PLATFORM>(),
        0
    };
    return cl::Context(device, props, NULL);
}

static bool unitTestsEnabled = false;

void enableUnitTests()
{
    unitTestsEnabled = true;
}

// Build a program that has already been constructed
static void buildProgram(
    cl::Program &program,
    const VECTOR_CLASS<cl::Device> &devices, const std::string &filename, const std::string &options)
{
    bool failed = false;
    try
    {
        int status = program.build(devices, options.c_str());
        if (status != 0)
            failed = true;
    }
    catch (cl::Error &e)
    {
        failed = true;
    }

    if (failed)
    {
        std::ostringstream msg;
        msg << "Internal error compiling " << filename << '\n';
        for (std::vector<cl::Device>::const_iterator device = devices.begin(); device != devices.end(); ++device)
        {
            const std::string log = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(*device);
            if (log != "" && log != "\n")
            {
                msg << "Log for device " << device->getInfo<CL_DEVICE_NAME>() << '\n';
                msg << log << '\n';
            }
        }
        throw InternalError(msg.str());
    }
}

cl::Program build(
    const cl::Context &context,
    const cl::Device &device,
    const std::string &filename,
    const std::map<std::string, int> &defines,
    const std::map<std::string, std::string> &stringDefines,
    const std::string &options)
{
    VECTOR_CLASS<cl::Device> devices(1, device);

    const std::map<std::string, Source> &sourceMap = detail::getSourceMap();
    assert(sourceMap.count(filename));
    const Source &source = sourceMap.find(filename)->second;

    std::ostringstream s;
    s.imbue(std::locale::classic());
    for (std::map<std::string, int>::const_iterator i = defines.begin(); i != defines.end(); ++i)
    {
        s << "#define " << i->first << " " << i->second << "\n";
    }
    for (std::map<std::string, std::string>::const_iterator i = stringDefines.begin();
         i != stringDefines.end(); ++i)
    {
        s << "#define " << i->first << " " << i->second << "\n";
    }
    if (unitTestsEnabled)
        s << "#define UNIT_TESTS 1\n";
    s << "#line 1 \"" << filename << "\"\n";

    /* First check if it is cached */
    KernelParameters::Key key;
    key.device = deviceKey(device);
    key.header = s.str();
    key.checksum = source.checksum;
    KernelParameters::Value value;
    if (getDB().kernel.lookup(key, value))
    {
        cl::Program::Binaries binaries(1);
        binaries[0] = std::make_pair(static_cast<const void *>(&value.binary[0]), value.binary.size());
        try
        {
            cl::Program program(context, devices, binaries, NULL);
            buildProgram(program, devices, filename, options);
            return program;
        }
        catch (InternalError &e)
        {
            // fall back to source if building failed
        }
        catch (cl::Error &e)
        {
            if (e.err() != CL_INVALID_BINARY)
                throw;
        }
    }

    cl::Program::Sources sources(2);
    const std::string header = s.str();
    sources[0] = std::make_pair(header.data(), header.length());
    sources[1] = std::make_pair(source.text.data(), source.text.length());

    cl::Program program(context, sources);

    buildProgram(program, devices, filename, options);

    /* Update the cache. The cl.hpp interface (as of 1.2.6) is fundamentally
     * not exception-safe, so we do things at the C API level.
     */
    VECTOR_CLASS< ::size_t> binarySizes = program.getInfo<CL_PROGRAM_BINARY_SIZES>();
    assert(binarySizes.size() == 1);

    value.binary.resize(binarySizes[0]);
    std::vector<unsigned char *> dataPtrs(1, &value.binary[0]);

    int err = clGetProgramInfo(program(), CL_PROGRAM_BINARIES,
                               sizeof(unsigned char *), &dataPtrs[0], NULL);
    if (err != CL_SUCCESS)
        throw cl::Error(err, "clGetProgramInfo");
    getDB().kernel.add(key, value);

    return program;
}

} // namespace detail
} // namespace clogs
