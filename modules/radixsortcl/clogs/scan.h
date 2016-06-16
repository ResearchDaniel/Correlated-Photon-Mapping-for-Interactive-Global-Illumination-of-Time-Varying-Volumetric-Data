/* Copyright (c) 2012, 2014 University of Cape Town
 * Copyright (c) 2014, Bruce Merry
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
 * Scan primitive.
 */

#ifndef CLOGS_SCAN_H
#define CLOGS_SCAN_H

#include <clogs/visibility_push.h>
#include <modules/opencl/cl.hpp>
#include <cstddef>
#include <clogs/visibility_pop.h>
#include <warn/push>
#include <warn/ignore/conversion>
#include <warn/ignore/dll-interface-base>
#include <clogs/core.h>
#include <clogs/platform.h>
#include <clogs/tune.h>
#include <warn/pop>

namespace clogs
{

class ScanProblem;

namespace detail
{
    class ScanProblem;
    class Scan;

    const ScanProblem &getDetail(const clogs::ScanProblem &);
} // namespace detail

/**
 * Encapsulates the specifics of a scan problem. After construction, use
 * methods (particularly @ref setType) to configure the scan.
 */
class CLOGS_API ScanProblem
{
private:
    detail::ScanProblem *detail_;
    friend const detail::ScanProblem &detail::getDetail(const clogs::ScanProblem &);

public:
    ScanProblem();
    ~ScanProblem();
    ScanProblem(const ScanProblem &);
    ScanProblem &operator=(const ScanProblem &);

    /**
     * Set the element type for the scan.
     *
     * @param type      The element type
     * @throw std::invalid_argument if @a type is not an integral scalar or vector type
     */
    void setType(const Type &type);

    /**
     * Set the autotuning policy.
     */
    void setTunePolicy(const TunePolicy &tunePolicy);
};

/**
 * Exclusive scan (prefix sum) primitive.
 *
 * One instance of this class can be reused for multiple scans, provided that
 *  - calls to @ref enqueue(const cl::CommandQueue &, const cl::Buffer &, const cl::Buffer &, ::size_t, const void *, const VECTOR_CLASS<cl::Event> *, cl::Event *) "enqueue" do not overlap; and
 *  - their execution does not overlap.
 *
 * An instance of the class is specialized to a specific context, device, and
 * type of value to scan. Any CL integral scalar or vector type can
 * be used.
 *
 * The implementation is based on the reduce-then-scan strategy described at
 * https://sites.google.com/site/duanemerrill/ScanTR2.pdf?attredirects=0
 */
class CLOGS_API Scan : public Algorithm
{
private:
    detail::Scan *getDetail() const;
    detail::Scan *getDetailNonNull() const;
    void construct(cl_context context, cl_device_id device, const ScanProblem &problem,
                   cl_int &err, const char *&errStr);
    friend void swap(Scan &, Scan &);

protected:
    void enqueue(cl_command_queue commandQueue,
                 cl_mem inBuffer,
                 cl_mem outBuffer,
                 ::size_t elements,
                 const void *offset,
                 cl_uint numEvents,
                 const cl_event *events,
                 cl_event *event,
                 cl_int &err,
                 const char *&errStr);


    void enqueue(cl_command_queue commandQueue,
                 cl_mem inBuffer,
                 cl_mem outBuffer,
                 ::size_t elements,
                 cl_mem offsetBuffer,
                 cl_uint offsetIndex,
                 cl_uint numEvents,
                 const cl_event *events,
                 cl_event *event,
                 cl_int &err,
                 const char *&errStr);

    void moveAssign(Scan &other);

public:
    /**
     * Default constructor. The object cannot be used in this state.
     */
    Scan();

#ifdef CLOGS_HAVE_RVALUE_REFERENCES
    Scan(Scan &&other) CLOGS_NOEXCEPT
    {
        moveConstruct(other);
    }

    Scan &operator=(Scan &&other) CLOGS_NOEXCEPT
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
     * @param type                 %Type of the values to scan.
     *
     * @throw std::invalid_argument if @a type is not an integral type supported on the device.
     * @throw clogs::InternalError if there was a problem with initialization.
     *
     * @deprecated This interface is deprecated as it does not scale with future feature
     * additions. Use the constructor taking a @ref clogs::ScanProblem instead.
     */
    Scan(const cl::Context &context, const cl::Device &device, const Type &type)
    {
        cl_int err;
        const char *errStr;
        ScanProblem problem;
        problem.setType(type);
        construct(context(), device(), problem, err, errStr);
        detail::handleError(err, errStr);
    }

    /**
     * Constructor.
     *
     * @param context              OpenCL context to use
     * @param device               OpenCL device to use.
     * @param problem              Description of the specific scan problem.
     *
     * @throw std::invalid_argument if @a problem is not supported on the device or is not initialized.
     * @throw clogs::InternalError if there was a problem with initialization.
     */
    Scan(const cl::Context &context, const cl::Device &device, const ScanProblem &problem)
    {
        cl_int err;
        const char *errStr;
        construct(context(), device(), problem, err, errStr);
        detail::handleError(err, errStr);
    }

    ~Scan(); ///< Destructor

    /**
     * Enqueue a scan operation on a command queue (in-place).
     *
     * This is equivalent to calling
     * @c enqueue(@a commandQueue, @a buffer, @a buffer, @a elements, @a offset, @a events, @a event);
     */
    void enqueue(const cl::CommandQueue &commandQueue,
                 const cl::Buffer &buffer,
                 ::size_t elements,
                 const void *offset = NULL,
                 const VECTOR_CLASS<cl::Event> *events = NULL,
                 cl::Event *event = NULL)
    {
        enqueue(commandQueue, buffer, buffer, elements, offset, events, event);
    }

    /**
     * Enqueue a scan operation on a command queue.
     *
     * An initial offset may optionally be passed in @a offset, which will be
     * added to all elements of the result. The pointer must point to the
     * type of element specified to the constructor. If no offset is desired,
     * @c NULL may be passed instead.
     *
     * The input and output buffers may be the same to do an in-place scan.
     *
     * @param commandQueue         The command queue to use.
     * @param inBuffer             The buffer to scan.
     * @param outBuffer            The buffer to fill with output.
     * @param elements             The number of elements to scan.
     * @param offset               The offset to add to all elements, or @c NULL.
     * @param events               Events to wait for before starting.
     * @param event                Event that will be signaled on completion.
     *
     * @throw cl::Error            If @a inBuffer is not readable on the device.
     * @throw cl::Error            If @a outBuffer is not writable on the device.
     * @throw cl::Error            If the element range overruns the buffer.
     * @throw cl::Error            If @a elements is zero.
     * @pre
     * - @a commandQueue was created with the context and device given to the constructor.
     * @post
     * - After execution, element @c i will be replaced by the sum of all elements strictly
     *   before @c i, plus the @a offset (if any).
     */
    void enqueue(const cl::CommandQueue &commandQueue,
                 const cl::Buffer &inBuffer,
                 const cl::Buffer &outBuffer,
                 ::size_t elements,
                 const void *offset = NULL,
                 const VECTOR_CLASS<cl::Event> *events = NULL,
                 cl::Event *event = NULL)
    {
        cl_event outEvent;
        cl_int err;
        const char *errStr;
        detail::UnwrapArray<cl::Event> events_(events);
        enqueue(commandQueue(), inBuffer(), outBuffer(), elements, offset,
                events_.size(), events_.data(),
                event != NULL ? &outEvent : NULL,
                err, errStr);
        detail::handleError(err, errStr);
        if (event != NULL)
            *event = outEvent; // steals reference
    }

    /// @overload
    void enqueue(cl_command_queue commandQueue,
                 cl_mem inBuffer,
                 cl_mem outBuffer,
                 ::size_t elements,
                 const void *offset = NULL,
                 cl_uint numEvents = 0,
                 const cl_event *events = NULL,
                 cl_event *event = NULL)
    {
        cl_int err;
        const char *errStr;
        enqueue(commandQueue, inBuffer, outBuffer, elements, offset, numEvents, events, event,
                err, errStr);
        detail::handleError(err, errStr);
    }

    /**
     * Enqueue a scan operation on a command queue, with an offset in a buffer (in-place).
     *
     * This is equivalent to calling
     * @c enqueue(@a commandQueue, @a buffer, @a buffer, @a elements, @a offsetBuffer, @a offsetIndex);
     */
    void enqueue(const cl::CommandQueue &commandQueue,
                 const cl::Buffer &buffer,
                 ::size_t elements,
                 const cl::Buffer &offsetBuffer,
                 cl_uint offsetIndex,
                 const VECTOR_CLASS<cl::Event> *events = NULL,
                 cl::Event *event = NULL)
    {
        enqueue(commandQueue, buffer, buffer, elements, offsetBuffer, offsetIndex, events, event);
    }

    /**
     * Enqueue a scan operation on a command queue, with an offset in a buffer.
     *
     * The offset is of the same type as the elements to be scanned, and is
     * stored in a buffer. It is added to all elements of the result. It is legal
     * for the offset to be in the same buffer as the values to scan, and it may
     * even be safely overwritten by the scan (it will be read before being
     * overwritten). This makes it possible to use do multi-pass algorithms with
     * variable output. The counting pass fills in the desired allocations, a
     * scan is used with one extra element at the end to hold the grand total,
     * and the subsequent passes use this extra element as the offset.
     *
     * The input and output buffers may be the same to do an in-place scan.
     *
     * @param commandQueue         The command queue to use.
     * @param inBuffer             The buffer to scan.
     * @param outBuffer            The buffer to fill with output.
     * @param elements             The number of elements to scan.
     * @param offsetBuffer         Buffer containing a value to add to all elements.
     * @param offsetIndex          Index (in units of the scan type) into @a offsetBuffer.
     * @param events               Events to wait for before starting.
     * @param event                Event that will be signaled on completion.
     *
     * @throw cl::Error            If @a inBuffer is not readable on the device.
     * @throw cl::Error            If @a outBuffer is not writable on the device.
     * @throw cl::Error            If the element range overruns the buffer.
     * @throw cl::Error            If @a elements is zero.
     * @throw cl::Error            If @a offsetBuffer is not readable.
     * @throw cl::Error            If @a offsetIndex overruns @a offsetBuffer.
     * @pre
     * - @a commandQueue was created with the context and device given to the constructor.
     * @post
     * - After execution, element @c i will be replaced by the sum of all elements strictly
     *   before @c i, plus the offset.
     */
    void enqueue(const cl::CommandQueue &commandQueue,
                 const cl::Buffer &inBuffer,
                 const cl::Buffer &outBuffer,
                 ::size_t elements,
                 const cl::Buffer &offsetBuffer,
                 cl_uint offsetIndex,
                 const VECTOR_CLASS<cl::Event> *events = NULL,
                 cl::Event *event = NULL)
    {
        cl_event outEvent;
        cl_int err;
        const char *errStr;
        detail::UnwrapArray<cl::Event> events_(events);
        enqueue(commandQueue(), inBuffer(), outBuffer(), elements,
                offsetBuffer(), offsetIndex,
                events_.size(), events_.data(),
                event != NULL ? &outEvent : NULL,
                err, errStr);
        detail::handleError(err, errStr);
        if (event != NULL)
            *event = outEvent; // steals reference
    }

    /// @overload
    void enqueue(cl_command_queue commandQueue,
                 cl_mem inBuffer,
                 cl_mem outBuffer,
                 ::size_t elements,
                 cl_mem offsetBuffer,
                 cl_uint offsetIndex,
                 cl_uint numEvents = 0,
                 const cl_event *events = NULL,
                 cl_event *event = NULL)
    {
        cl_int err;
        const char *errStr;
        enqueue(commandQueue, inBuffer, outBuffer, elements, offsetBuffer, offsetIndex,
                numEvents, events, event, err, errStr);
        detail::handleError(err, errStr);
    }
};

void swap(Scan &a, Scan &b);

} // namespace clogs

#endif /* !CLOGS_SCAN_H */
