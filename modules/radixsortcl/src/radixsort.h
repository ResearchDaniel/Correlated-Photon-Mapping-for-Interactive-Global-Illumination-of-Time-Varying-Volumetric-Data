/* Copyright (c) 2012-2014 University of Cape Town
 * Copyright (c) 2014, 2015 Bruce Merry
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
 * Internals of the radix-sort class.
 */

#ifndef RADIXSORT_H
#define RADIXSORT_H

#include "clhpp11.h"

#include <clogs/visibility_push.h>
#include <cstddef>
#include <utility>
#include <boost/any.hpp>
#include <clogs/visibility_pop.h>

#include <clogs/core.h>
#include "parameters.h"
#include "cache_types.h"
#include "utils.h"
#include "tune.h"

class TestRadixsort;

namespace clogs
{
namespace detail
{

class Radixsort;

class CLOGS_LOCAL RadixsortProblem
{
private:
    friend class Radixsort;

    Type keyType;
    Type valueType;
    TunePolicy tunePolicy;

public:
    void setKeyType(const Type &keyType);
    void setValueType(const Type &valueType);
    void setTunePolicy(const TunePolicy &tunePolicy);
};

/**
 * Radix-sort implementation.
 * @see clogs::Radixsort.
 */
class CLOGS_LOCAL Radixsort : public Algorithm
{
    friend class ::TestRadixsort;
private:
    ::size_t reduceWorkGroupSize;    ///< Work group size for the initial reduce phase
    ::size_t scanWorkGroupSize;      ///< Work group size for the middle scan phase
    ::size_t scatterWorkGroupSize;   ///< Work group size for the final scatter phase
    ::size_t scatterWorkScale;       ///< Elements per work item for the final scan/scatter phase
    ::size_t scatterSlice;           ///< Number of work items that cooperate
    ::size_t scanBlocks;             ///< Maximum number of items in the middle phase
    ::size_t keySize;                ///< Size of the key type
    ::size_t valueSize;              ///< Size of the value type
    unsigned int radix;              ///< Sort radix
    unsigned int radixBits;          ///< Number of bits forming radix
    cl::Program program;             ///< Program containing the kernels
    cl::Kernel reduceKernel;         ///< Initial reduction kernel
    cl::Kernel scanKernel;           ///< Middle-phase scan kernel
    cl::Kernel scatterKernel;        ///< Final scan/scatter kernel
    cl::Buffer histogram;            ///< Histogram of the blocks by radix
    cl::Buffer tmpKeys;              ///< User-provided buffer to hold temporary keys
    cl::Buffer tmpValues;            ///< User-provided buffer to hold temporary values

    ::size_t getTileSize() const;
    ::size_t getBlockSize(::size_t elements) const;
    ::size_t getBlocks(::size_t elements, ::size_t len) const;

    /**
     * Enqueue the reduction kernel.
     * @param queue                Command queue to enqueue to.
     * @param out                  Histogram table, with storage for @a len * @ref radix uints.
     * @param in                   Keys to sort.
     * @param len                  Length of each block to reduce.
     * @param elements             Number of elements to reduce.
     * @param firstBit             Index of first bit forming radix.
     * @param events               Events to wait for (if not @c NULL).
     * @param[out] event           Event for this work (if not @c NULL).
     */
    void enqueueReduce(
        const cl::CommandQueue &queue, const cl::Buffer &out, const cl::Buffer &in,
        ::size_t len, ::size_t elements, unsigned int firstBit,
        const VECTOR_CLASS<cl::Event> *events, cl::Event *event);

    /**
     * Enqueue the scan kernel.
     * @param queue                Command queue to enqueue to.
     * @param histogram            Histogram of @ref scanBlocks * @ref radix elements, block-major.
     * @param blocks               Actual number of blocks to scan
     * @param events               Events to wait for (if not @c NULL).
     * @param[out] event           Event for this work (if not @c NULL).
     */
    void enqueueScan(
        const cl::CommandQueue &queue, const cl::Buffer &histogram, ::size_t blocks,
        const VECTOR_CLASS<cl::Event> *events, cl::Event *event);

    /**
     * Enqueue the scatter kernel.
     * @param queue                Command queue to enqueue to.
     * @param outKeys              Output buffer for partitioned keys.
     * @param outValues            Output buffer for parititoned values.
     * @param inKeys               Input buffer with unsorted keys.
     * @param inValues             Input buffer with values corresponding to @a inKeys.
     * @param histogram            Scanned histogram of @ref scanBlocks * @ref radix elements, block-major.
     * @param len                  Length of each block to reduce.
     * @param elements             Total number of key/value pairs.
     * @param firstBit             Index of first bit to sort on.
     * @param events               Events to wait for (if not @c NULL).
     * @param[out] event           Event for this work (if not @c NULL).
     *
     * @pre The input and output buffers must all be distinct.
     */
    void enqueueScatter(
        const cl::CommandQueue &queue, const cl::Buffer &outKeys, const cl::Buffer &outValues,
        const cl::Buffer &inKeys, const cl::Buffer &inValues, const cl::Buffer &histogram,
        ::size_t len, ::size_t elements, unsigned int firstBit,
        const VECTOR_CLASS<cl::Event> *events, cl::Event *event);

    /**
     * Second construction phase. This is called either by the normal constructor
     * or during autotuning.
     *
     * @param context, device, problem  Constructor arguments
     * @param params                    Autotuned parameters
     */
    void initialize(
        const cl::Context &context, const cl::Device &device,
        const RadixsortProblem &problem,
        const RadixsortParameters::Value &params);

    /**
     * Constructor for autotuning
     */
    Radixsort(const cl::Context &context, const cl::Device &device,
              const RadixsortProblem &problem,
              const RadixsortParameters::Value &params);

    static std::pair<double, double> tuneReduceCallback(
        const cl::Context &context, const cl::Device &device,
        std::size_t elements, const boost::any &params,
        const RadixsortProblem &problem);

    static std::pair<double, double> tuneScatterCallback(
        const cl::Context &context, const cl::Device &device,
        std::size_t elements, const boost::any &params,
        const RadixsortProblem &problem);

    static std::pair<double, double> tuneBlocksCallback(
        const cl::Context &context, const cl::Device &device,
        std::size_t elements, const boost::any &params,
        const RadixsortProblem &problem);

    /**
     * Returns key for looking up autotuning parameters.
     *
     * @param device, problem  Constructor parameters.
     */
    static RadixsortParameters::Key makeKey(const cl::Device &device, const RadixsortProblem &problem);

    /**
     * Perform autotuning.
     *
     * @param device, problem Constructor parameters
     */
    static RadixsortParameters::Value tune(
        const cl::Device &device,
        const RadixsortProblem &problem);

public:
    /**
     * Constructor.
     * @see @ref clogs::Radixsort::Radixsort(const cl::Context &, const cl::Device &, const RadixsortProblem &).
     */
    Radixsort(const cl::Context &context, const cl::Device &device, const RadixsortProblem &problem);

    /**
     * Enqueue a scan operation on a command queue.
     * @see @ref clogs::Radixsort::enqueue.
     */
    void enqueue(const cl::CommandQueue &commandQueue,
                 const cl::Buffer &keys, const cl::Buffer &values,
                 ::size_t elements, unsigned int maxBits = 0,
                 const VECTOR_CLASS<cl::Event> *events = NULL,
                 cl::Event *event = NULL);

    /**
     * Set temporary buffers used during sorting.
     * @see #clogs::Radixsort::setTemporaryBuffers.
     */
    void setTemporaryBuffers(const cl::Buffer &keys, const cl::Buffer &values);

    /**
     * Return whether a type is supported as a key type on a device.
     */
    static bool keyTypeSupported(const cl::Device &device, const Type &keyType);

    /**
     * Return whether a type is supported as a value type on a device.
     */
    static bool valueTypeSupported(const cl::Device &device, const Type &valueType);
};

} // namespace detail
} // namespace clogs

#endif /* RADIXSORT_H */
