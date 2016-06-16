/* Copyright (c) 2014, 2015 Bruce Merry
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
 * Reduce implementation.
 */

/**
 * @file
 *
 * Scan implementation.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "clhpp11.h"

#include <clogs/visibility_push.h>
#include <cstddef>
#include <map>
#include <string>
#include <cassert>
#include <vector>
#include <algorithm>
#include <utility>
#include <clogs/visibility_pop.h>

#include <clogs/core.h>
#include <clogs/reduce.h>
#include "reduce.h"
#include "utils.h"
#include "parameters.h"
#include "tune.h"
#include "cache.h"
#include <inviwo/core/util/clock.h>
namespace clogs
{

namespace detail
{

void ReduceProblem::setType(const Type &type)
{
    if (type.getBaseType() == TYPE_VOID)
        throw std::invalid_argument("type must not be void");
    this->type = type;
}

void ReduceProblem::setTunePolicy(const TunePolicy &tunePolicy)
{
    this->tunePolicy = tunePolicy;
}


void Reduce::initialize(
    const cl::Context &context, const cl::Device &device,
    const ReduceProblem &problem,
    const ReduceParameters::Value &params)
{
    reduceWorkGroupSize = params.reduceWorkGroupSize;
    reduceBlocks = params.reduceBlocks;
    elementSize = problem.type.getSize();

    std::map<std::string, int> defines;
    std::map<std::string, std::string> stringDefines;
    if (problem.type.getBaseType() == TYPE_HALF)
        defines["ENABLE_KHR_FP16"] = 1;
    if (problem.type.getBaseType() == TYPE_DOUBLE)
        defines["ENABLE_KHR_FP64"] = 1;
    defines["REDUCE_WORK_GROUP_SIZE"] = reduceWorkGroupSize;
    defines["REDUCE_BLOCKS"] = reduceBlocks;
    stringDefines["REDUCE_T"] = problem.type.getName();

    try
    {
        cl_uint wgcInit = reduceBlocks;
        // The extra element is used for storing the final reduction to be read back
        sums = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, (reduceBlocks + 1) * elementSize);
        wgc = cl::Buffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_uint), &wgcInit);

        program = build(context, device, "reduce.cl", defines, stringDefines);

        reduceKernel = cl::Kernel(program, "reduce");
        reduceKernel.setArg(0, wgc);
        reduceKernel.setArg(6, sums);
    }
    catch (cl::Error &e)
    {
        throw InternalError(std::string("Error preparing kernels for reduce: ") + e.what());
    }
}

std::pair<double, double> Reduce::tuneReduceCallback(
    const cl::Context &context, const cl::Device &device,
    std::size_t elements, const boost::any &paramsAny,
    const ReduceProblem &problem)
{
    const ReduceParameters::Value &params = boost::any_cast<const ReduceParameters::Value &>(paramsAny);
    const ::size_t reduceWorkGroupSize = params.reduceWorkGroupSize;
    const ::size_t reduceBlocks = params.reduceBlocks;
    const ::size_t elementSize = problem.type.getSize();
    const ::size_t allocSize = elements * elementSize;
    cl::Buffer buffer(context, CL_MEM_READ_ONLY, allocSize);
    cl::Buffer output(context, CL_MEM_WRITE_ONLY, elementSize);
    cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE);
    cl::Event event;

    ::size_t blockSize = roundUp(elements, reduceWorkGroupSize * reduceBlocks) / reduceBlocks;

    Reduce reduce(context, device, problem, params);
    // Warmup pass
    reduce.enqueue(queue, buffer, output, 0, elements, 0);
    queue.finish();
    // Timing pass
    reduce.enqueue(queue, buffer, output, 0, elements, 0, NULL, &event);
    queue.finish();

    event.wait();
    cl_ulong start = event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
    cl_ulong end = event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
    double elapsed = end - start;
    double rate = reduceBlocks * blockSize / elapsed;
    return std::make_pair(rate, rate * 1.05);
}

ReduceParameters::Value Reduce::tune(
    const cl::Device &device, const ReduceProblem &problem)
{
    const TunePolicy &policy = problem.tunePolicy;
    policy.assertEnabled();
    std::ostringstream description;
    description << "reduce for " << problem.type.getName() << " elements";
    policy.logStartAlgorithm(description.str(), device);

    const ::size_t elementSize = problem.type.getSize();
    const ::size_t localMemElements = device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() / elementSize;
    const ::size_t maxWorkGroupSize = std::min(device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>(), localMemElements);
    const ::size_t computeUnits = device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
    const ::size_t startBlocks = 16 * computeUnits;

    std::vector<std::size_t> problemSizes;
    problemSizes.push_back(65536);
    problemSizes.push_back(32 * 1024 * 1024 / elementSize);

    ReduceParameters::Value cand;
    cand.reduceBlocks = startBlocks;
    {
        // Tune work group size
        std::vector<boost::any> sets;
        for (::size_t reduceWorkGroupSize = 1; reduceWorkGroupSize <= maxWorkGroupSize; reduceWorkGroupSize *= 2)
        {
            ReduceParameters::Value params = cand;
            params.reduceWorkGroupSize = reduceWorkGroupSize;
            sets.push_back(params);
        }

        using namespace FUNCTIONAL_NAMESPACE::placeholders;
        cand = boost::any_cast<ReduceParameters::Value>(tuneOne(
                policy, device, sets, problemSizes,
                FUNCTIONAL_NAMESPACE::bind(&Reduce::tuneReduceCallback, _1, _2, _3, _4, problem)));
    }

    {
        // Tune number of blocks
        std::vector<boost::any> sets;
        for (::size_t blocks = 4 * computeUnits; blocks <= 64 * computeUnits; blocks += 4 * computeUnits)
        {
            ReduceParameters::Value params = cand;
            params.reduceBlocks = blocks;
            sets.push_back(params);
        }

        using namespace FUNCTIONAL_NAMESPACE::placeholders;
        cand = boost::any_cast<ReduceParameters::Value>(tuneOne(
                policy, device, sets, problemSizes,
                FUNCTIONAL_NAMESPACE::bind(&Reduce::tuneReduceCallback, _1, _2, _3, _4, problem)));
    }

    policy.logEndAlgorithm();
    return cand;
}

bool Reduce::typeSupported(const cl::Device &device, const Type &type)
{
    return type.isComputable(device) && type.isStorable(device);
}

Reduce::Reduce(const cl::Context &context, const cl::Device &device, const ReduceProblem &problem)
{
    if (!typeSupported(device, problem.type))
        throw std::invalid_argument("type is not a supported format on this device");

    ReduceParameters::Key key = makeKey(device, problem);
    ReduceParameters::Value params;
    if (!getDB().reduce.lookup(key, params))
    {
        params = tune(device, problem);
        getDB().reduce.add(key, params);
    }
    initialize(context, device, problem, params);
}

Reduce::Reduce(const cl::Context &context, const cl::Device &device, const ReduceProblem &problem,
               const ReduceParameters::Value &params)
{
    initialize(context, device, problem, params);
}

ReduceParameters::Key Reduce::makeKey(const cl::Device &device, const ReduceProblem &problem)
{
    /* To reduce the amount of time for tuning, we assume that signed
     * and unsigned variants are equivalent, and canonicalise to signed.
     */
    Type canon;
    switch (problem.type.getBaseType())
    {
    case TYPE_UCHAR:
        canon = Type(TYPE_CHAR, problem.type.getLength());
        break;
    case TYPE_USHORT:
        canon = Type(TYPE_SHORT, problem.type.getLength());
        break;
    case TYPE_UINT:
        canon = Type(TYPE_INT, problem.type.getLength());
        break;
    case TYPE_ULONG:
        canon = Type(TYPE_LONG, problem.type.getLength());
        break;
    default:
        canon = problem.type;
    }

    ReduceParameters::Key key;
    key.device = deviceKey(device);
    key.elementType = canon.getName();
    return key;
}

void Reduce::enqueue(
    const cl::CommandQueue &commandQueue,
    const cl::Buffer &inBuffer,
    const cl::Buffer &outBuffer,
    ::size_t first,
    ::size_t elements,
    ::size_t outPosition,
    const VECTOR_CLASS<cl::Event> *events,
    cl::Event *event)
{
    
    /* Validate parameters */
    if (first + elements < first)
    {
        // Only happens if first + elements overflows. size_t is unsigned so behaviour
        // is well-defined.
        throw cl::Error(CL_INVALID_VALUE, "clogs::Reduce::enqueue: range out of input buffer bounds");
    }
    if (inBuffer.getInfo<CL_MEM_SIZE>() / elementSize < first + elements)
        throw cl::Error(CL_INVALID_VALUE, "clogs::Reduce::enqueue: range out of input buffer bounds");
    if (outBuffer.getInfo<CL_MEM_SIZE>() / elementSize <= outPosition)
        throw cl::Error(CL_INVALID_VALUE, "clogs::Reduce::enqueue: output position out of buffer bounds");
    if (!(inBuffer.getInfo<CL_MEM_FLAGS>() & (CL_MEM_READ_WRITE | CL_MEM_READ_ONLY)))
    {
        throw cl::Error(CL_INVALID_VALUE, "clogs::Reduce::enqueue: input buffer is not readable");
    }
    if (!(outBuffer.getInfo<CL_MEM_FLAGS>() & (CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY)))
    {
        throw cl::Error(CL_INVALID_VALUE, "clogs::Reduce::enqueue: output buffer is not writable");
    }
    if (elements == 0)
        throw cl::Error(CL_INVALID_GLOBAL_WORK_SIZE, "clogs::Reduce::enqueue: elements is zero");

    const ::size_t blockSize = roundUp(elements, reduceWorkGroupSize * reduceBlocks) / reduceBlocks;
    
    reduceKernel.setArg(1, outBuffer);
    reduceKernel.setArg(2, (cl_uint) outPosition);
    reduceKernel.setArg(3, inBuffer);
    reduceKernel.setArg(4, (cl_uint) first);
    reduceKernel.setArg(5, (cl_uint) elements);
    reduceKernel.setArg(7, (cl_uint) blockSize);

    cl::Event reduceEvent;
    commandQueue.enqueueNDRangeKernel(
        reduceKernel,
        cl::NullRange,
        cl::NDRange(reduceWorkGroupSize * reduceBlocks),
        cl::NDRange(reduceWorkGroupSize),
        events, &reduceEvent);
    doEventCallback(reduceEvent);

    if (event != NULL)
        *event = reduceEvent;
}

void Reduce::enqueue(
    const cl::CommandQueue &commandQueue,
    bool blocking,
    const cl::Buffer &inBuffer,
    void *out,
    ::size_t first,
    ::size_t elements,
    const VECTOR_CLASS<cl::Event> *events,
    cl::Event *event, cl::Event *reduceEvent)
{
    VECTOR_CLASS<cl::Event> reduceEvents(1);
    cl::Event readEvent;
    
    if (out == NULL)
        throw cl::Error(CL_INVALID_VALUE, "clogs::Reduce::enqueue: out is NULL");

    //{inviwo::ScopedClockCPU clock("Reduce", "Reduce", -0.1f);
    enqueue(commandQueue, inBuffer, sums, first, elements, reduceBlocks, events, &reduceEvents[0]);
    //auto buf = commandQueue.enqueueMapBuffer(sums, true, CL_MAP_READ, reduceBlocks * elementSize, elementSize, &reduceEvents,
    //    &readEvent);
    //memcpy(buf, out, elementSize);
    //commandQueue.enqueueUnmapMemObject(sums, buf);
    commandQueue.enqueueReadBuffer(
        sums, blocking,
        reduceBlocks * elementSize,
        elementSize,
        out,
        &reduceEvents,
        &readEvent);
    //}
    doEventCallback(readEvent);
    if (event != NULL)
        *event = readEvent;
    if (reduceEvent != NULL)
        *reduceEvent = reduceEvents[0];
}

const ReduceProblem &getDetail(const clogs::ReduceProblem &problem)
{
    return *problem.detail_;
}

} // namespace detail

ReduceProblem::ReduceProblem() : detail_(new detail::ReduceProblem())
{
}

ReduceProblem::~ReduceProblem()
{
    delete detail_;
}

ReduceProblem::ReduceProblem(const ReduceProblem &other)
    : detail_(new detail::ReduceProblem(*other.detail_))
{
}

ReduceProblem &ReduceProblem::operator=(const ReduceProblem &other)
{
    if (detail_ != other.detail_)
    {
        detail::ReduceProblem *tmp = new detail::ReduceProblem(*other.detail_);
        delete detail_;
        detail_ = tmp;
    }
    return *this;
}

void ReduceProblem::setType(const Type &type)
{
    assert(detail_ != NULL);
    detail_->setType(type);
}

void ReduceProblem::setTunePolicy(const TunePolicy &tunePolicy)
{
    assert(detail_ != NULL);
    detail_->setTunePolicy(detail::getDetail(tunePolicy));
}


Reduce::Reduce()
{
}

detail::Reduce *Reduce::getDetail() const
{
    return static_cast<detail::Reduce *>(Algorithm::getDetail());
}

detail::Reduce *Reduce::getDetailNonNull() const
{
    return static_cast<detail::Reduce *>(Algorithm::getDetailNonNull());
}

void Reduce::construct(cl_context context, cl_device_id device, const ReduceProblem &problem,
                       cl_int &err, const char *&errStr)
{
    try
    {
        setDetail(new detail::Reduce(
            detail::retainWrap<cl::Context>(context),
            detail::retainWrap<cl::Device>(device),
            detail::getDetail(problem)));
        detail::clearError(err, errStr);
    }
    catch (cl::Error &e)
    {
        detail::setError(err, errStr, e);
    }
}

void Reduce::moveAssign(Reduce &other)
{
    delete static_cast<detail::Reduce *>(Algorithm::moveAssign(other));
}

Reduce::~Reduce()
{
    delete getDetail();
}

void Reduce::enqueue(cl_command_queue commandQueue,
                     cl_mem inBuffer,
                     cl_mem outBuffer,
                     ::size_t first,
                     ::size_t elements,
                     ::size_t outPosition,
                     cl_uint numEvents,
                     const cl_event *events,
                     cl_event *event,
                     cl_int &err,
                     const char *&errStr)
{
    try
    {
        VECTOR_CLASS<cl::Event> events_ = detail::retainWrap<cl::Event>(numEvents, events);
        cl::Event event_;
        getDetailNonNull()->enqueue(
            detail::retainWrap<cl::CommandQueue>(commandQueue),
            detail::retainWrap<cl::Buffer>(inBuffer),
            detail::retainWrap<cl::Buffer>(outBuffer),
            first, elements, outPosition,
            events ? &events_ : NULL,
            event ? &event_ : NULL);
        detail::clearError(err, errStr);
        detail::unwrap(event_, event);
    }
    catch (cl::Error &e)
    {
        detail::setError(err, errStr, e);
    }
}

void Reduce::enqueue(cl_command_queue commandQueue,
                     bool blocking,
                     cl_mem inBuffer,
                     void *out,
                     ::size_t first,
                     ::size_t elements,
                     cl_uint numEvents,
                     const cl_event *events,
                     cl_event *event,
                     cl_event *reduceEvent,
                     cl_int &err,
                     const char *&errStr)
{
    try
    {
        VECTOR_CLASS<cl::Event> events_ = detail::retainWrap<cl::Event>(numEvents, events);
        cl::Event event_;
        cl::Event reduceEvent_;
        getDetailNonNull()->enqueue(
            detail::retainWrap<cl::CommandQueue>(commandQueue),
            blocking,
            detail::retainWrap<cl::Buffer>(inBuffer),
            out, first, elements,
            events ? &events_ : NULL,
            event ? &event_ : NULL,
            reduceEvent ? &reduceEvent_ : NULL);
        detail::clearError(err, errStr);
        detail::unwrap(event_, event);
        detail::unwrap(reduceEvent_, reduceEvent);
    }
    catch (cl::Error &e)
    {
        detail::setError(err, errStr, e);
    }
}

void swap(Reduce &a, Reduce &b)
{
    a.swap(b);
}

} // namespace clogs
