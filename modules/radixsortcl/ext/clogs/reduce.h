/* Copyright (c) 2014, Bruce Merry
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
 * Reduce primitive.
 */

#ifndef CLOGS_REDUCE_H
#define CLOGS_REDUCE_H

#include <clogs/visibility_push.h>
#include <modules/opencl/cl.hpp>
#include <cstddef>
#include <clogs/visibility_pop.h>

#include <clogs/core.h>
#include <clogs/platform.h>
#include <clogs/tune.h>

namespace clogs
{

class ReduceProblem;

namespace detail
{
    class Reduce;
    class ReduceProblem;

    const ReduceProblem &getDetail(const clogs::ReduceProblem &);
} // namespace detail

class Reduce;

/**
 * Encapsulates the specifics of a reduction problem. After construction, use
 * methods (particularly @ref setType) to configure the reduction.
 */
class CLOGS_API ReduceProblem
{
private:
    detail::ReduceProblem *detail_;
    friend const detail::ReduceProblem &detail::getDetail(const clogs::ReduceProblem &);

public:
    ReduceProblem();
    ~ReduceProblem();
    ReduceProblem(const ReduceProblem &);
    ReduceProblem &operator=(const ReduceProblem &);

    /**
     * Set the element type for the reduction.
     *
     * @param type      The element type
     */
    void setType(const Type &type);

    /**
     * Set the autotuning policy.
     */
    void setTunePolicy(const TunePolicy &tunePolicy);
};

/**
 * Reduction primitive.
 *
 * One instance of this class can be reused for multiple scans, provided that
 *  - calls to @ref enqueue(const cl::CommandQueue &, const cl::Buffer &, const cl::Buffer &, ::size_t, ::size_t elements, ::size_t outPosition, const VECTOR_CLASS<cl::Event> *, cl::Event *) "enqueue" do not overlap; and
 *  - their execution does not overlap.
 *
 * An instance of the class is specialized to a specific context, device, and
 * type of value to scan. Any CL integral scalar or vector type can
 * be used.
 *
 * The implementation divides the data into a number of blocks, each of which
 * is reduced by a work-group. The last work-group handles the final reduction.
 */
class CLOGS_API Reduce : public Algorithm
{
private:
    detail::Reduce *getDetail() const;
    detail::Reduce *getDetailNonNull() const;
    void construct(
        cl_context context, cl_device_id device, const ReduceProblem &problem,
        cl_int &err, const char *&errStr);
    void moveAssign(Reduce &other);
    friend void swap(Reduce &, Reduce &);

protected:
    void enqueue(cl_command_queue commandQueue,
                 cl_mem inBuffer,
                 cl_mem outBuffer,
                 ::size_t first,
                 ::size_t elements,
                 ::size_t outPosition,
                 cl_uint numEvents,
                 const cl_event *events,
                 cl_event *event,
                 cl_int &err,
                 const char *&errStr);

    void enqueue(cl_command_queue commandQueue,
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
                 const char *&errStr);

public:
    /**
     * Default constructor. The object cannot be used in this state.
     */
    Reduce();

#ifdef CLOGS_HAVE_RVALUE_REFERENCES
    Reduce(Reduce &&other) CLOGS_NOEXCEPT
    {
        moveConstruct(other);
    }

    Reduce &operator=(Reduce &&other) CLOGS_NOEXCEPT
    {
        moveAssign(other);
        return *this;
    }
#endif

    /**
     * Constructor.
     *
     * @param context              OpenCL context to use
     * @param device               OpenCL device to use.
     * @param problem              Description of the specific reduction problem.
     *
     * @throw std::invalid_argument if @a problem is not supported on the device or is not initialized.
     * @throw clogs::InternalError if there was a problem with initialization.
     */
    Reduce(const cl::Context &context, const cl::Device &device, const ReduceProblem &problem)
    {
        int err;
        const char *errStr;
        construct(context(), device(), problem, err, errStr);
        detail::handleError(err, errStr);
    }

    /**
     * Constructor. This class will add new references to the @a context and @a device.
     *
     * @param context              OpenCL context to use
     * @param device               OpenCL device to use.
     * @param problem              Description of the specific reduction problem.
     *
     * @throw std::invalid_argument if @a problem is not supported on the device or is not initialized.
     * @throw clogs::InternalError if there was a problem with initialization.
     */
    Reduce(cl_context context, cl_device_id device, const ReduceProblem &problem)
    {
        int err;
        const char *errStr;
        construct(context, device, problem, err, errStr);
        detail::handleError(err, errStr);
    }

    ~Reduce(); ///< Destructor

    /**
     * Enqueue a reduction operation on a command queue.
     *
     * @param commandQueue         The command queue to use.
     * @param inBuffer             The buffer to reduce.
     * @param outBuffer            The buffer to which the result is written.
     * @param first                The index (in elements, not bytes) to begin the reduction.
     * @param elements             The number of elements to reduce.
     * @param outPosition          The index (in elements, not bytes) at which to write the result.
     * @param events               Events to wait for before starting.
     * @param event                Event that will be signaled on completion.
     *
     * @throw cl::Error            If @a inBuffer is not readable on the device
     * @throw cl::Error            If @a outBuffer is not writable on the devic
     * @throw cl::Error            If the element range overruns the buffer.
     * @throw cl::Error            If @a elements is zero.
     *
     * @pre
     * - @a commandQueue was created with the context and device given to the constructor.
     * - The output does not overlap with the input.
     */
    void enqueue(const cl::CommandQueue &commandQueue,
                 const cl::Buffer &inBuffer,
                 const cl::Buffer &outBuffer,
                 ::size_t first,
                 ::size_t elements,
                 ::size_t outPosition,
                 const VECTOR_CLASS<cl::Event> *events = NULL,
                 cl::Event *event = NULL)
    {
        cl_event outEvent;
        int err;
        const char *errStr;
        detail::UnwrapArray<cl::Event> rawEvents(events);
        enqueue(commandQueue(), inBuffer(), outBuffer(), first, elements, outPosition,
                rawEvents.size(), rawEvents.data(),
                event != NULL ? &outEvent : NULL,
                err, errStr);
        detail::handleError(err, errStr);
        if (event != NULL)
            *event = outEvent; // steals the reference
    }

    /// @overload
    void enqueue(cl_command_queue commandQueue,
                 cl_mem inBuffer,
                 cl_mem outBuffer,
                 ::size_t first,
                 ::size_t elements,
                 ::size_t outPosition,
                 cl_uint numEvents = 0,
                 const cl_event *events = NULL,
                 cl_event *event = NULL)
    {
        cl_int err;
        const char *errStr;
        enqueue(commandQueue, inBuffer, outBuffer, first, elements, outPosition,
                numEvents, events, event, err, errStr);
        detail::handleError(err, errStr);
    }

    /**
     * Enqueue a reduction operation and read the result back to the host.
     * This is a convenience wrapper that avoids the need to separately
     * call @c clEnqueueReadBuffer.
     */
    void enqueue(const cl::CommandQueue &commandQueue,
                 bool blocking,
                 const cl::Buffer &inBuffer,
                 void *out,
                 ::size_t first,
                 ::size_t elements,
                 const VECTOR_CLASS<cl::Event> *events = NULL,
                 cl::Event *event = NULL, cl::Event *reduceEvent = NULL)
    {
        cl_event outEvent;
        cl_event reduceOutEvent;
        cl_int err;
        const char *errStr;
        detail::UnwrapArray<cl::Event> rawEvents(events);
        enqueue(commandQueue(), blocking, inBuffer(), out, first, elements,
                rawEvents.size(), rawEvents.data(),
                event != NULL ? &outEvent : NULL,
                reduceEvent != NULL ? &reduceOutEvent : NULL,
                err, errStr);
        detail::handleError(err, errStr);
        if (event != NULL)
            *event = outEvent; // steals the reference
        if (reduceEvent != NULL)
            *reduceEvent = reduceOutEvent; // steals the reference
    }

    /// @overload
    void enqueue(cl_command_queue commandQueue,
                 bool blocking,
                 cl_mem inBuffer,
                 void *out,
                 ::size_t first,
                 ::size_t elements,
                 cl_uint numEvents = 0,
                 const cl_event *events = NULL,
                 cl_event *event = NULL, cl_event *reduceEvent = NULL)
    {
        cl_int err;
        const char *errStr;
        enqueue(commandQueue, blocking, inBuffer, out, first, elements,
            numEvents, events, event, reduceEvent, err, errStr);
        detail::handleError(err, errStr);
    }
};

void swap(Reduce &a, Reduce &b);

} // namespace clogs

#endif /* !CLOGS_REDUCE_H */
