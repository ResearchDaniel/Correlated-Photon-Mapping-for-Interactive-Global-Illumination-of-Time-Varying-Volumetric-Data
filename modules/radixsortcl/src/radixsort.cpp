/* Copyright (c) 2012-2014 University of Cape Town
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
 * Radixsort implementation.
 */

#include "clhpp11.h"

#include <clogs/visibility_push.h>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <cassert>
#include <climits>
#include <algorithm>
#include <vector>
#include <utility>
#include <clogs/visibility_pop.h>

#include <clogs/core.h>
#include <clogs/radixsort.h>
#include "utils.h"
#include "radixsort.h"
#include "parameters.h"
#include "tune.h"
#include "cache.h"
#include "tr1_random.h"
#include "tr1_functional.h"

namespace clogs
{
namespace detail
{

void RadixsortProblem::setKeyType(const Type &keyType)
{
    if (!(keyType.isIntegral()
          && !keyType.isSigned()
          && keyType.getLength() == 1))
        throw std::invalid_argument("keyType is not valid");
    this->keyType = keyType;
}

void RadixsortProblem::setValueType(const Type &valueType)
{
    this->valueType = valueType;
}

void RadixsortProblem::setTunePolicy(const TunePolicy &tunePolicy)
{
    this->tunePolicy = tunePolicy;
}


::size_t Radixsort::getTileSize() const
{
    return std::max(reduceWorkGroupSize, scatterWorkScale * scatterWorkGroupSize);
}

::size_t Radixsort::getBlockSize(::size_t elements) const
{
    const ::size_t tileSize = getTileSize();
    return (elements + tileSize * scanBlocks - 1) / (tileSize * scanBlocks) * tileSize;
}

::size_t Radixsort::getBlocks(::size_t elements, ::size_t len) const
{
    const ::size_t slicesPerWorkGroup = scatterWorkGroupSize / scatterSlice;
    ::size_t blocks = (elements + len - 1) / len;
    blocks = roundUp(blocks, slicesPerWorkGroup);
    assert(blocks <= scanBlocks);
    return blocks;
}

void Radixsort::enqueueReduce(
    const cl::CommandQueue &queue, const cl::Buffer &out, const cl::Buffer &in,
    ::size_t len, ::size_t elements, unsigned int firstBit,
    const VECTOR_CLASS<cl::Event> *events, cl::Event *event)
{
    reduceKernel.setArg(0, out);
    reduceKernel.setArg(1, in);
    reduceKernel.setArg(2, (cl_uint) len);
    reduceKernel.setArg(3, (cl_uint) elements);
    reduceKernel.setArg(4, (cl_uint) firstBit);
    cl_uint blocks = getBlocks(elements, len);
    cl::Event reduceEvent;
    queue.enqueueNDRangeKernel(reduceKernel,
                               cl::NullRange,
                               cl::NDRange(reduceWorkGroupSize * blocks),
                               cl::NDRange(reduceWorkGroupSize),
                               events, &reduceEvent);
    doEventCallback(reduceEvent);
    if (event != NULL)
        *event = reduceEvent;
}

void Radixsort::enqueueScan(
    const cl::CommandQueue &queue, const cl::Buffer &histogram, ::size_t blocks,
    const VECTOR_CLASS<cl::Event> *events, cl::Event *event)
{
    scanKernel.setArg(0, histogram);
    scanKernel.setArg(1, (cl_uint) blocks);
    cl::Event scanEvent;
    queue.enqueueNDRangeKernel(scanKernel,
                               cl::NullRange,
                               cl::NDRange(scanWorkGroupSize),
                               cl::NDRange(scanWorkGroupSize),
                               events, &scanEvent);
    doEventCallback(scanEvent);
    if (event != NULL)
        *event = scanEvent;
}

void Radixsort::enqueueScatter(
    const cl::CommandQueue &queue, const cl::Buffer &outKeys, const cl::Buffer &outValues,
    const cl::Buffer &inKeys, const cl::Buffer &inValues, const cl::Buffer &histogram,
    ::size_t len, ::size_t elements, unsigned int firstBit,
    const VECTOR_CLASS<cl::Event> *events, cl::Event *event)
{
    scatterKernel.setArg(0, outKeys);
    scatterKernel.setArg(1, inKeys);
    scatterKernel.setArg(2, histogram);
    scatterKernel.setArg(3, (cl_uint) len);
    scatterKernel.setArg(4, (cl_uint) elements);
    scatterKernel.setArg(5, (cl_uint) firstBit);
    if (valueSize != 0)
    {
        scatterKernel.setArg(6, outValues);
        scatterKernel.setArg(7, inValues);
    }
    const ::size_t blocks = getBlocks(elements, len);
    const ::size_t slicesPerWorkGroup = scatterWorkGroupSize / scatterSlice;
    assert(blocks % slicesPerWorkGroup == 0);
    const ::size_t workGroups = blocks / slicesPerWorkGroup;
    cl::Event scatterEvent;
    queue.enqueueNDRangeKernel(scatterKernel,
                               cl::NullRange,
                               cl::NDRange(scatterWorkGroupSize * workGroups),
                               cl::NDRange(scatterWorkGroupSize),
                               events, &scatterEvent);
    doEventCallback(scatterEvent);
    if (event != NULL)
        *event = scatterEvent;
}

void Radixsort::enqueue(
    const cl::CommandQueue &queue,
    const cl::Buffer &keys, const cl::Buffer &values,
    ::size_t elements, unsigned int maxBits,
    const VECTOR_CLASS<cl::Event> *events,
    cl::Event *event)
{
    /* Validate parameters */
    if (keys.getInfo<CL_MEM_SIZE>() < elements * keySize)
    {
        throw cl::Error(CL_INVALID_VALUE, "clogs::Radixsort::enqueue: range of out of buffer bounds for key");
    }
    if (valueSize != 0 && values.getInfo<CL_MEM_SIZE>() < elements * valueSize)
    {
        throw cl::Error(CL_INVALID_VALUE, "clogs::Radixsort::enqueue: range of out of buffer bounds for value");
    }
    if (!(keys.getInfo<CL_MEM_FLAGS>() & CL_MEM_READ_WRITE))
    {
        throw cl::Error(CL_INVALID_VALUE, "clogs::Radixsort::enqueue: keys is not read-write");
    }
    if (valueSize != 0 && !(values.getInfo<CL_MEM_FLAGS>() & CL_MEM_READ_WRITE))
    {
        throw cl::Error(CL_INVALID_VALUE, "clogs::Radixsort::enqueue: values is not read-write");
    }

    if (elements == 0)
        throw cl::Error(CL_INVALID_GLOBAL_WORK_SIZE, "clogs::Radixsort::enqueue: elements is zero");
    if (maxBits == 0)
        maxBits = CHAR_BIT * keySize;
    else if (maxBits > CHAR_BIT * keySize)
        throw cl::Error(CL_INVALID_VALUE, "clogs::Radixsort::enqueue: maxBits is too large");

    const cl::Context &context = queue.getInfo<CL_QUEUE_CONTEXT>();

    // If necessary, allocate temporary buffers for ping-pong
    cl::Buffer tmpKeys, tmpValues;
    if (this->tmpKeys() && this->tmpKeys.getInfo<CL_MEM_SIZE>() >= elements * keySize)
        tmpKeys = this->tmpKeys;
    else
        tmpKeys = cl::Buffer(context, CL_MEM_READ_WRITE, elements * keySize);
    if (valueSize != 0)
    {
        if (this->tmpValues() && this->tmpValues.getInfo<CL_MEM_SIZE>() >= elements * valueSize)
            tmpValues = this->tmpValues;
        else
            tmpValues = cl::Buffer(context, CL_MEM_READ_WRITE, elements * valueSize);
    }

    cl::Event next;
    std::vector<cl::Event> prev(1);
    const std::vector<cl::Event> *waitFor = events;
    const cl::Buffer *curKeys = &keys;
    const cl::Buffer *curValues = &values;
    const cl::Buffer *nextKeys = &tmpKeys;
    const cl::Buffer *nextValues = &tmpValues;

    const ::size_t blockSize = getBlockSize(elements);
    const ::size_t blocks = getBlocks(elements, blockSize);
    assert(blocks <= scanBlocks);

    for (unsigned int firstBit = 0; firstBit < maxBits; firstBit += radixBits)
    {
        enqueueReduce(queue, histogram, *curKeys, blockSize, elements, firstBit, waitFor, &next);
        prev[0] = next; waitFor = &prev;
        enqueueScan(queue, histogram, blocks, waitFor, &next);
        prev[0] = next; waitFor = &prev;
        enqueueScatter(queue, *nextKeys, *nextValues, *curKeys, *curValues, histogram, blockSize,
                       elements, firstBit, waitFor, &next);
        prev[0] = next; waitFor = &prev;
        std::swap(curKeys, nextKeys);
        std::swap(curValues, nextValues);
    }
    if (curKeys != &keys)
    {
        /* Odd number of ping-pongs, so we have to copy back again.
         * We don't actually need to serialize the copies, but it simplifies the event
         * management.
         */
        queue.enqueueCopyBuffer(*curKeys, *nextKeys, 0, 0, elements * keySize, waitFor, &next);
        doEventCallback(next);
        prev[0] = next; waitFor = &prev;
        if (valueSize != 0)
        {
            queue.enqueueCopyBuffer(*curValues, *nextValues, 0, 0, elements * valueSize, waitFor, &next);
            doEventCallback(next);
            prev[0] = next; waitFor = &prev;
        }
    }
    if (event != NULL)
        *event = next;
}

void Radixsort::setTemporaryBuffers(const cl::Buffer &keys, const cl::Buffer &values)
{
    tmpKeys = keys;
    tmpValues = values;
}

void Radixsort::initialize(
    const cl::Context &context, const cl::Device &device,
    const RadixsortProblem &problem,
    const RadixsortParameters::Value &params)
{
    reduceWorkGroupSize = params.reduceWorkGroupSize;
    scanWorkGroupSize = params.scanWorkGroupSize;
    scatterWorkGroupSize = params.scatterWorkGroupSize;
    scatterWorkScale = params.scatterWorkScale;
    scanBlocks = params.scanBlocks;
    keySize = problem.keyType.getSize();
    valueSize = problem.valueType.getSize();
    radixBits = params.radixBits;

    radix = 1U << radixBits;
    scatterSlice = std::max(params.warpSizeSchedule, ::size_t(radix));

    std::map<std::string, int> defines;
    std::map<std::string, std::string> stringDefines;
    defines["WARP_SIZE_MEM"] = params.warpSizeMem;
    defines["WARP_SIZE_SCHEDULE"] = params.warpSizeSchedule;
    defines["REDUCE_WORK_GROUP_SIZE"] = reduceWorkGroupSize;
    defines["SCAN_WORK_GROUP_SIZE"] = scanWorkGroupSize;
    defines["SCATTER_WORK_GROUP_SIZE"] = scatterWorkGroupSize;
    defines["SCATTER_WORK_SCALE"] = scatterWorkScale;
    defines["SCATTER_SLICE"] = scatterSlice;
    defines["SCAN_BLOCKS"] = scanBlocks;
    defines["RADIX_BITS"] = radixBits;
    stringDefines["KEY_T"] = problem.keyType.getName();
    if (problem.valueType.getBaseType() != TYPE_VOID)
    {
        /* There are cases (at least on NVIDIA) where value types have
         * different performance even when they are the same size e.g. uchar3
         * vs uint. Avoid this by canonicalising the value type. This has the
         * extra benefit that there are fewer possible kernels.
         */
        Type kernelValueType = problem.valueType;
        switch (valueSize)
        {
        case 1: kernelValueType = TYPE_UCHAR; break;
        case 2: kernelValueType = TYPE_USHORT; break;
        case 4: kernelValueType = TYPE_UINT; break;
        case 8: kernelValueType = TYPE_ULONG; break;
        case 16: kernelValueType = Type(TYPE_UINT, 4); break;
        case 32: kernelValueType = Type(TYPE_UINT, 8); break;
        case 64: kernelValueType = Type(TYPE_UINT, 16); break;
        case 128: kernelValueType = Type(TYPE_ULONG, 16); break;
        }
        assert(kernelValueType.getSize() == valueSize);

        stringDefines["VALUE_T"] = kernelValueType.getName();
    }

    /* Generate code for upsweep and downsweep. This is done here rather
     * than relying on loop unrolling, constant folding and so on because
     * compilers don't always figure that out correctly (particularly when
     * it comes to an inner loop whose trip count depends on the counter
     * from an outer loop.
     */
    std::vector<std::string> upsweepStmts, downsweepStmts;
    std::vector< ::size_t> stops;
    stops.push_back(1);
    stops.push_back(radix);
    if (scatterSlice > radix)
        stops.push_back(scatterSlice);
    stops.push_back(scatterSlice * radix);
    for (int i = int(stops.size()) - 2; i >= 0; i--)
    {
        ::size_t from = stops[i + 1];
        ::size_t to = stops[i];
        if (to >= scatterSlice)
        {
            std::string toStr = detail::toString(to);
            std::string fromStr = detail::toString(from);
            upsweepStmts.push_back("upsweepMulti(wg->hist.level1.i, wg->hist.level2.c + "
                                   + toStr + ", " + fromStr + ", " + toStr + ", lid);");
            downsweepStmts.push_back("downsweepMulti(wg->hist.level1.i, wg->hist.level2.c + "
                                   + toStr + ", " + fromStr + ", " + toStr + ", lid);");
        }
        else
        {
            while (from >= to * 4)
            {
                std::string fromStr = detail::toString(from);
                std::string toStr = detail::toString(from / 4);
                bool forceZero = (from == 4);
                upsweepStmts.push_back("upsweep4(wg->hist.level2.i + " + toStr + ", wg->hist.level2.c + "
                                       + toStr + ", " + toStr + ", lid, SCATTER_SLICE);");
                downsweepStmts.push_back("downsweep4(wg->hist.level2.i + " + toStr + ", wg->hist.level2.c + "
                                       + toStr + ", " + toStr + ", lid, SCATTER_SLICE, "
                                       + (forceZero ? "true" : "false") + ");");
                from /= 4;
            }
            if (from == to * 2)
            {
                std::string fromStr = detail::toString(from);
                std::string toStr = detail::toString(from / 2);
                bool forceZero = (from == 2);
                upsweepStmts.push_back("upsweep2(wg->hist.level2.s + " + toStr + ", wg->hist.level2.c + "
                                       + toStr + ", " + toStr + ", lid, SCATTER_SLICE);");
                downsweepStmts.push_back("downsweep2(wg->hist.level2.s + " + toStr + ", wg->hist.level2.c + "
                                       + toStr + ", " + toStr + ", lid, SCATTER_SLICE, "
                                       + (forceZero ? "true" : "false") + ");");
            }
        }
    }
    std::ostringstream upsweep, downsweep;
    upsweep << "do { ";
    for (std::size_t i = 0; i < upsweepStmts.size(); i++)
        upsweep << upsweepStmts[i];
    upsweep << " } while (0)";
    downsweep << "do { ";
    for (int i = int(downsweepStmts.size()) - 1; i >= 0; i--)
        downsweep << downsweepStmts[i];
    downsweep << "} while (0)";
    stringDefines["UPSWEEP()"] = upsweep.str();
    stringDefines["DOWNSWEEP()"] = downsweep.str();

    try
    {
        histogram = cl::Buffer(context, CL_MEM_READ_WRITE, params.scanBlocks * radix * sizeof(cl_uint));
        program = build(context, device, "radixsort.cl", defines, stringDefines);

        reduceKernel = cl::Kernel(program, "radixsortReduce");

        scanKernel = cl::Kernel(program, "radixsortScan");
        scanKernel.setArg(0, histogram);

        scatterKernel = cl::Kernel(program, "radixsortScatter");
        scatterKernel.setArg(1, histogram);
    }
    catch (cl::Error &e)
    {
        throw InternalError(std::string("Error preparing kernels for radixsort: ") + e.what());
    }
}

Radixsort::Radixsort(
    const cl::Context &context, const cl::Device &device,
    const RadixsortProblem &problem,
    const RadixsortParameters::Value &params)
{
    initialize(context, device, problem, params);
}

Radixsort::Radixsort(
    const cl::Context &context, const cl::Device &device,
    const RadixsortProblem &problem)
{
    if (!keyTypeSupported(device, problem.keyType))
        throw std::invalid_argument("keyType is not valid");
    if (!valueTypeSupported(device, problem.valueType))
        throw std::invalid_argument("valueType is not valid");

    RadixsortParameters::Key key = makeKey(device, problem);
    RadixsortParameters::Value params;
    if (!getDB().radixsort.lookup(key, params))
    {
        params = tune(device, problem);
        getDB().radixsort.add(key, params);
    }
    initialize(context, device, problem, params);
}

RadixsortParameters::Key Radixsort::makeKey(
    const cl::Device &device,
    const RadixsortProblem &problem)
{
    RadixsortParameters::Key key;
    key.device = deviceKey(device);
    key.keyType = problem.keyType.getName();
    key.valueSize = problem.valueType.getSize();
    return key;
}

bool Radixsort::keyTypeSupported(const cl::Device &device, const Type &keyType)
{
    return keyType.isIntegral()
        && !keyType.isSigned()
        && keyType.getLength() == 1
        && keyType.isComputable(device)
        && keyType.isStorable(device);
}

bool Radixsort::valueTypeSupported(const cl::Device &device, const Type &valueType)
{
    return valueType.getBaseType() == TYPE_VOID
        || valueType.isStorable(device);
}

static cl::Buffer makeRandomBuffer(const cl::CommandQueue &queue, ::size_t size)
{
    cl::Buffer buffer(queue.getInfo<CL_QUEUE_CONTEXT>(), CL_MEM_READ_WRITE, size);
    cl_uchar *data = reinterpret_cast<cl_uchar *>(
        queue.enqueueMapBuffer(buffer, CL_TRUE, CL_MAP_WRITE, 0, size));
    RANDOM_NAMESPACE::mt19937 engine;
    for (::size_t i = 0; i < size; i++)
    {
        /* We take values directly from the engine rather than using a
         * distribution, because the engine is guaranteed to be portable
         * across compilers.
         */
        data[i] = engine() & 0xFF;
    }
    queue.enqueueUnmapMemObject(buffer, data);
    return buffer;
}

std::pair<double, double> Radixsort::tuneReduceCallback(
    const cl::Context &context, const cl::Device &device,
    std::size_t elements, const boost::any &paramsAny,
    const RadixsortProblem &problem)
{
    const RadixsortParameters::Value &params = boost::any_cast<const RadixsortParameters::Value &>(paramsAny);
    cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE);
    const ::size_t keyBufferSize = elements * problem.keyType.getSize();
    const cl::Buffer keyBuffer = makeRandomBuffer(queue, keyBufferSize);

    Radixsort sort(context, device, problem, params);
    const ::size_t blockSize = sort.getBlockSize(elements);
    // Warmup
    sort.enqueueReduce(queue, sort.histogram, keyBuffer, blockSize, elements, 0, NULL, NULL);
    queue.finish();
    // Timing pass
    cl::Event event;
    sort.enqueueReduce(queue, sort.histogram, keyBuffer, blockSize, elements, 0, NULL, &event);
    queue.finish();

    event.wait();
    cl_ulong start = event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
    cl_ulong end = event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
    double elapsed = end - start;
    double rate = elements / elapsed;
    return std::make_pair(rate, rate);
}

std::pair<double, double> Radixsort::tuneScatterCallback(
    const cl::Context &context, const cl::Device &device,
    std::size_t elements, const boost::any &paramsAny,
    const RadixsortProblem &problem)
{
    const RadixsortParameters::Value &params = boost::any_cast<const RadixsortParameters::Value &>(paramsAny);
    cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE);
    const ::size_t keyBufferSize = elements * problem.keyType.getSize();
    const ::size_t valueBufferSize = elements * problem.valueType.getSize();
    const cl::Buffer keyBuffer = makeRandomBuffer(queue, keyBufferSize);
    const cl::Buffer outKeyBuffer(context, CL_MEM_READ_WRITE, keyBufferSize);
    cl::Buffer valueBuffer, outValueBuffer;
    if (problem.valueType.getBaseType() != TYPE_VOID)
    {
        valueBuffer = makeRandomBuffer(queue, valueBufferSize);
        outValueBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, valueBufferSize);
    }

    Radixsort sort(context, device, problem, params);
    const ::size_t blockSize = sort.getBlockSize(elements);
    const ::size_t blocks = sort.getBlocks(elements, blockSize);

    // Prepare histogram
    sort.enqueueReduce(queue, sort.histogram, keyBuffer, blockSize, elements, 0, NULL, NULL);
    sort.enqueueScan(queue, sort.histogram, blocks, NULL, NULL);
    // Warmup
    sort.enqueueScatter(
        queue,
        outKeyBuffer, outValueBuffer,
        keyBuffer, valueBuffer,
        sort.histogram, blockSize, elements, 0, NULL, NULL);
    queue.finish();
    // Timing pass
    cl::Event event;
    sort.enqueueScatter(
        queue,
        outKeyBuffer, outValueBuffer,
        keyBuffer, valueBuffer,
        sort.histogram, blockSize, elements, 0, NULL, &event);
    queue.finish();

    event.wait();
    cl_ulong start = event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
    cl_ulong end = event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
    double elapsed = end - start;
    double rate = elements / elapsed;
    return std::make_pair(rate, rate);
}

std::pair<double, double> Radixsort::tuneBlocksCallback(
    const cl::Context &context, const cl::Device &device,
    std::size_t elements, const boost::any &paramsAny,
    const RadixsortProblem &problem)
{
    const RadixsortParameters::Value &params = boost::any_cast<const RadixsortParameters::Value &>(paramsAny);
    cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE);
    const ::size_t keyBufferSize = elements * problem.keyType.getSize();
    const ::size_t valueBufferSize = elements * problem.valueType.getSize();
    const cl::Buffer keyBuffer = makeRandomBuffer(queue, keyBufferSize);
    const cl::Buffer outKeyBuffer(context, CL_MEM_READ_WRITE, keyBufferSize);
    cl::Buffer valueBuffer, outValueBuffer;
    if (problem.valueType.getBaseType() != TYPE_VOID)
    {
        valueBuffer = makeRandomBuffer(queue, valueBufferSize);
        outValueBuffer = cl::Buffer(context, CL_MEM_READ_WRITE, valueBufferSize);
    }

    Radixsort sort(context, device, problem, params);
    const ::size_t blockSize = sort.getBlockSize(elements);
    const ::size_t blocks = sort.getBlocks(elements, blockSize);

    cl::Event reduceEvent;
    cl::Event scanEvent;
    cl::Event scatterEvent;
    // Warmup and real passes
    for (int pass = 0; pass < 2; pass++)
    {
        sort.enqueueReduce(queue, sort.histogram, keyBuffer, blockSize, elements, 0, NULL, &reduceEvent);
        sort.enqueueScan(queue, sort.histogram, blocks, NULL, &scanEvent);
        sort.enqueueScatter(
            queue,
            outKeyBuffer, outValueBuffer,
            keyBuffer, valueBuffer,
            sort.histogram, blockSize, elements, 0,
            NULL, &scatterEvent);
        queue.finish();
    }

    reduceEvent.wait();
    scatterEvent.wait();
    cl_ulong start = reduceEvent.getProfilingInfo<CL_PROFILING_COMMAND_START>();
    cl_ulong end = scatterEvent.getProfilingInfo<CL_PROFILING_COMMAND_END>();
    double elapsed = end - start;
    double rate = elements / elapsed;
    // Fewer blocks means better performance on small problem sizes, so only
    // use more blocks if it makes a real improvement
    return std::make_pair(rate, rate * 1.05);
}

RadixsortParameters::Value Radixsort::tune(
    const cl::Device &device,
    const RadixsortProblem &problem)
{
    const TunePolicy &policy = problem.tunePolicy;
    policy.assertEnabled();
    std::ostringstream description;
    description << "radixsort for " << problem.keyType.getName() << " keys and "
        << problem.valueType.getSize() << " byte values";
    policy.logStartAlgorithm(description.str(), device);

    /* Limit memory usage, otherwise devices with lots of RAM will take a long
     * time to tune. For GPUs we need a large problem size to get accurate
     * statistics, but for CPUs we use less to keep tuning time down.
     */
    bool isCPU = device.getInfo<CL_DEVICE_TYPE>() & CL_DEVICE_TYPE_CPU;
    const ::size_t maxDataSize = isCPU ? 32 * 1024 * 1024 : 256 * 1024 * 1024;
    const ::size_t dataSize = std::min(maxDataSize, (std::size_t) device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() / 8);
    const ::size_t elements = dataSize / (problem.keyType.getSize() + problem.valueType.getSize());

    std::vector<std::size_t> problemSizes;
    if (elements > 1024 * 1024)
        problemSizes.push_back(1024 * 1024);
    problemSizes.push_back(elements);

    const ::size_t maxWorkGroupSize = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
    const ::size_t warpSizeMem = getWarpSizeMem(device);
    const ::size_t warpSizeSchedule = getWarpSizeSchedule(device);

    RadixsortParameters::Value out;
    // TODO: change to e.g. 2-6 after adding code to select the best one
    for (unsigned int radixBits = 4; radixBits <= 4; radixBits++)
    {
        const unsigned int radix = 1U << radixBits;
        const unsigned int scanWorkGroupSize = 4 * radix; // TODO: autotune it
        ::size_t maxBlocks =
            (device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() / sizeof(cl_uint) - 2 * scanWorkGroupSize) / radix;
        /* Work around devices like G80 lying about the maximum local memory
         * size, by starting with a smaller size.
         */
        ::size_t startBlocks = maxBlocks / 2;
        startBlocks = roundDown(startBlocks, (::size_t) scanWorkGroupSize / radix);

        if (maxWorkGroupSize < radix)
            break;

        RadixsortParameters::Value cand;
        // Set default values, which are later tuned
        const ::size_t scatterSlice = std::max(warpSizeSchedule, (::size_t) radix);
        cand.radixBits = radixBits;
        cand.warpSizeMem = warpSizeMem;
        cand.warpSizeSchedule = warpSizeSchedule;
        cand.scanBlocks = startBlocks;
        cand.scanWorkGroupSize = scanWorkGroupSize;
        cand.scatterWorkGroupSize = scatterSlice;
        cand.scatterWorkScale = 1;

        // Tune the reduction kernel, assuming a large scanBlocks
        {
            std::vector<boost::any> sets;
            for (::size_t reduceWorkGroupSize = radix; reduceWorkGroupSize <= maxWorkGroupSize; reduceWorkGroupSize *= 2)
            {
                RadixsortParameters::Value params = cand;
                params.reduceWorkGroupSize = reduceWorkGroupSize;
                sets.push_back(params);
            }
            using namespace FUNCTIONAL_NAMESPACE::placeholders;
            cand = boost::any_cast<RadixsortParameters::Value>(tuneOne(
                policy, device, sets, problemSizes,
                FUNCTIONAL_NAMESPACE::bind(&Radixsort::tuneReduceCallback, _1, _2, _3, _4, problem)));
        }

        // Tune the scatter kernel
        {
            std::vector<boost::any> sets;
            for (::size_t scatterWorkGroupSize = scatterSlice; scatterWorkGroupSize <= maxWorkGroupSize; scatterWorkGroupSize *= 2)
            {
                // TODO: increase search space
                for (::size_t scatterWorkScale = 1; scatterWorkScale <= 255 / scatterSlice; scatterWorkScale++)
                {
                    RadixsortParameters::Value params = cand;
                    const ::size_t slicesPerWorkGroup = scatterWorkGroupSize / scatterSlice;
                    params.scanBlocks = roundDown(startBlocks, slicesPerWorkGroup);
                    params.scatterWorkGroupSize = scatterWorkGroupSize;
                    params.scatterWorkScale = scatterWorkScale;
                    sets.push_back(params);
                }
            }
            using namespace FUNCTIONAL_NAMESPACE::placeholders;
            cand = boost::any_cast<RadixsortParameters::Value>(tuneOne(
                policy, device, sets, problemSizes,
                FUNCTIONAL_NAMESPACE::bind(&Radixsort::tuneScatterCallback, _1, _2, _3, _4, problem)));
        }

        // Tune the block count
        {
            std::vector<boost::any> sets;

            ::size_t scanWorkGroupSize = cand.scanWorkGroupSize;
            ::size_t scatterWorkGroupSize = cand.scatterWorkGroupSize;
            const ::size_t slicesPerWorkGroup = scatterWorkGroupSize / scatterSlice;
            // Have to reduce the maximum to align with slicesPerWorkGroup, which was 1 earlier
            maxBlocks = roundDown(maxBlocks, slicesPerWorkGroup);
            maxBlocks = roundDown(maxBlocks, scatterWorkGroupSize / radix);
            std::set< ::size_t> scanBlockCands;
            for (::size_t scanBlocks = std::max(scanWorkGroupSize / radix, slicesPerWorkGroup); scanBlocks <= maxBlocks; scanBlocks *= 2)
            {
                scanBlockCands.insert(scanBlocks);
            }
            /* Also try with block counts that are a multiple of the number of compute units,
             * which gives a more balanced work distribution.
             */
            for (::size_t scanBlocks = device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
                 scanBlocks <= maxBlocks; scanBlocks *= 2)
            {
                ::size_t blocks = roundDown(scanBlocks, slicesPerWorkGroup);
                if (blocks >= scanWorkGroupSize / radix)
                    scanBlockCands.insert(blocks);
            }
            // Finally, try the upper limit, in case performance is monotonic
            scanBlockCands.insert(maxBlocks);

            for (std::set< ::size_t>::const_iterator i = scanBlockCands.begin();
                 i != scanBlockCands.end(); ++i)
            {
                RadixsortParameters::Value params = cand;
                params.scanBlocks = *i;
                sets.push_back(params);
            }

            using namespace FUNCTIONAL_NAMESPACE::placeholders;
            cand = boost::any_cast<RadixsortParameters::Value>(tuneOne(
                policy, device, sets, problemSizes,
                FUNCTIONAL_NAMESPACE::bind(&Radixsort::tuneBlocksCallback, _1, _2, _3, _4, problem)));
        }

        // TODO: benchmark the whole combination
        out = cand;
    }

    policy.logEndAlgorithm();
    return out;
}

const RadixsortProblem &getDetail(const clogs::RadixsortProblem &problem)
{
    return *problem.detail_;
}

} // namespace detail

RadixsortProblem::RadixsortProblem() : detail_(new detail::RadixsortProblem())
{
}

RadixsortProblem::~RadixsortProblem()
{
    delete detail_;
}

RadixsortProblem::RadixsortProblem(const RadixsortProblem &other)
    : detail_(new detail::RadixsortProblem(*other.detail_))
{
}

RadixsortProblem &RadixsortProblem::operator=(const RadixsortProblem &other)
{
    if (detail_ != other.detail_)
    {
        detail::RadixsortProblem *tmp = new detail::RadixsortProblem(*other.detail_);
        delete detail_;
        detail_ = tmp;
    }
    return *this;
}

void RadixsortProblem::setKeyType(const Type &keyType)
{
    assert(detail_ != NULL);
    detail_->setKeyType(keyType);
}

void RadixsortProblem::setValueType(const Type &valueType)
{
    assert(detail_ != NULL);
    detail_->setValueType(valueType);
}

void RadixsortProblem::setTunePolicy(const TunePolicy &tunePolicy)
{
    assert(detail_ != NULL);
    detail_->setTunePolicy(detail::getDetail(tunePolicy));
}


Radixsort::Radixsort()
{
}

detail::Radixsort *Radixsort::getDetail() const
{
    return static_cast<detail::Radixsort *>(Algorithm::getDetail());
}

detail::Radixsort *Radixsort::getDetailNonNull() const
{
    return static_cast<detail::Radixsort *>(Algorithm::getDetailNonNull());
}

void Radixsort::construct(
    cl_context context, cl_device_id device,
    const RadixsortProblem &problem,
    cl_int &err, const char *&errStr)
{
    try
    {
        setDetail(new detail::Radixsort(
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

void Radixsort::moveAssign(Radixsort &other)
{
    delete static_cast<detail::Radixsort *>(Algorithm::moveAssign(other));
}

void Radixsort::enqueue(
    cl_command_queue commandQueue,
    cl_mem keys, cl_mem values,
    ::size_t elements, unsigned int maxBits,
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
            detail::retainWrap<cl::Buffer>(keys),
            detail::retainWrap<cl::Buffer>(values),
            elements, maxBits,
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

void Radixsort::setTemporaryBuffers(cl_mem keys, cl_mem values,
                                    cl_int &err, const char *&errStr)
{
    try
    {
        getDetailNonNull()->setTemporaryBuffers(
            detail::retainWrap<cl::Buffer>(keys),
            detail::retainWrap<cl::Buffer>(values));
        detail::clearError(err, errStr);
    }
    catch (cl::Error &e)
    {
        detail::setError(err, errStr, e);
    }
}

Radixsort::~Radixsort()
{
    delete getDetail();
}

void swap(Radixsort &a, Radixsort &b)
{
    a.swap(b);
}

} // namespace clogs
