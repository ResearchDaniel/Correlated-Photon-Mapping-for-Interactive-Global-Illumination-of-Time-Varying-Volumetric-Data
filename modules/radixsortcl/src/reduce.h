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

#ifndef REDUCE_H
#define REDUCE_H

#include "clhpp11.h"

#include <clogs/visibility_push.h>
#include <cstddef>
#include <cassert>
#include <vector>
#include <string>
#include <boost/any.hpp>
#include <clogs/visibility_pop.h>

#include <clogs/core.h>
#include "parameters.h"
#include "cache_types.h"
#include "utils.h"
#include "tune.h"

namespace clogs
{
namespace detail
{

class Reduce;

/**
 * Internal implementation of @ref clogs::ReduceProblem.
 */
class CLOGS_LOCAL ReduceProblem
{
private:
    friend class Reduce;
    Type type;
    TunePolicy tunePolicy;

public:
    void setType(const Type &type);
    void setTunePolicy(const TunePolicy &tunePolicy);
};

/**
 * Internal implementation of @ref clogs::Reduce.
 */
class CLOGS_LOCAL Reduce : public Algorithm
{
private:
    ::size_t reduceWorkGroupSize;
    ::size_t reduceBlocks;
    ::size_t elementSize;

    cl::Program program;
    cl::Kernel reduceKernel;

    cl::Buffer sums;
    cl::Buffer wgc;

    /**
     * Second construction phase. This is called either by the normal constructor
     * or during autotuning.
     *
     * @param context, device, problem Constructor arguments
     * @param params                   Autotuned parameters
     */
    void initialize(
        const cl::Context &context, const cl::Device &device, const ReduceProblem &problem,
        const ReduceParameters::Value &params);

    /**
     * Constructor for autotuning
     */
    Reduce(const cl::Context &context, const cl::Device &device, const ReduceProblem &problem,
           const ReduceParameters::Value &params);

    static std::pair<double, double> tuneReduceCallback(
        const cl::Context &context, const cl::Device &device,
        std::size_t elements, const boost::any &parameters,
        const ReduceProblem &problem);

    /**
     * Returns key for looking up autotuning parameters.
     */
    static ReduceParameters::Key makeKey(const cl::Device &device, const ReduceProblem &problem);

    /**
     * Perform autotuning.
     *
     * @param device      Device to tune for
     * @param problem     Problem parameters
     */
    static ReduceParameters::Value tune(
        const cl::Device &device, const ReduceProblem &problem);

public:
    /**
     * Constructor.
     * @see @ref clogs::Reduce::Reduce(const cl::Context &, const cl::Device &, const ReduceProblem &)
     */
    Reduce(const cl::Context &context, const cl::Device &device, const ReduceProblem &problem);

    /**
     * Enqueue a reduction on a command queue.
     * @see @ref clogs::Reduce::enqueue.
     */
    void enqueue(const cl::CommandQueue &commandQueue,
                 const cl::Buffer &inBuffer,
                 const cl::Buffer &outBuffer,
                 ::size_t first,
                 ::size_t elements,
                 ::size_t outPosition,
                 const VECTOR_CLASS<cl::Event> *events = NULL,
                 cl::Event *event = NULL);

    /**
     * Enqueue a reduction on a command queue and read the result back to the host.
     * @see @ref clogs::Reduce::enqueue.
     */
    void enqueue(const cl::CommandQueue &commandQueue,
                 bool blocking,
                 const cl::Buffer &inBuffer,
                 void *out,
                 ::size_t first,
                 ::size_t elements,
                 const VECTOR_CLASS<cl::Event> *events = NULL,
                 cl::Event *event = NULL, cl::Event *reduceEvent = NULL);

    /**
     * Return whether a type is supported on a device.
     */
    static bool typeSupported(const cl::Device &device, const Type &type);
};

} // namespace detail
} // namespace clogs

#endif /* REDUCE_H */
