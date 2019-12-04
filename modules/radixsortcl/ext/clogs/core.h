/* Copyright (c) 2012-2013 University of Cape Town
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
 * Basic types shared by several primitives.
 */

#ifndef CLOGS_CORE_H
#define CLOGS_CORE_H

#include <clogs/visibility_push.h>
#include <modules/opencl/cl.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#include <clogs/visibility_pop.h>
#include <clogs/platform.h>

/**
 * OpenCL primitives.
 *
 * The primary classes of interest are @ref Scan, @ref Radixsort, and @ref
 * Reduce, which provide the algorithms. The other classes are utilities and
 * helpers.
 */
namespace clogs
{

/// API major version number.
#define CLOGS_VERSION_MAJOR 1
/// API minor version number.
#define CLOGS_VERSION_MINOR 5

/**
 * Exception thrown on internal errors that are not the user's fault.
 */
class CLOGS_API InternalError : public std::runtime_error
{
public:
    InternalError(const std::string &msg);
};

/**
 * Exception thrown when the autotuning cache could not be read.
 */
class CLOGS_API CacheError : public InternalError
{
public:
    CacheError(const std::string &msg);
};

/**
 * Exception thrown when a configuration could not be tuned at all.
 */
class CLOGS_API TuneError : public InternalError
{
public:
    TuneError(const std::string &msg);
};


#ifdef __CL_ENABLE_EXCEPTIONS
typedef cl::Error Error;
#else
class CLOGS_LOCAL Error
{
private:
    cl_int err_;
    const char *errStr_;

public:
    explicit Error(cl_int err, const char *errStr = NULL) : err_(err), errStr_(errStr)
    {
    }

    virtual const char *what() const
    {
        return errStr_ != NULL ? errStr_ : "";
    }

    cl_int err() const
    {
        return err_;
    }
};
#endif // !__CL_ENABLE_EXCEPTIONS

/**
 * Enumeration of scalar types supported by OpenCL C which can be stored in a buffer.
 */
enum CLOGS_API BaseType
{
    TYPE_VOID,
    TYPE_UCHAR,
    TYPE_CHAR,
    TYPE_USHORT,
    TYPE_SHORT,
    TYPE_UINT,
    TYPE_INT,
    TYPE_ULONG,
    TYPE_LONG,
    TYPE_HALF,
    TYPE_FLOAT,
    TYPE_DOUBLE
};

/**
 * Encapsulation of an OpenCL built-in type that can be stored in a buffer.
 *
 * An instance of this class can represent either a scalar, a vector, or the
 * @c void type.
 */
class CLOGS_API Type
{
private:
    BaseType baseType;    ///< Element type
    unsigned int length;  ///< Vector length (1 for scalars)

public:
    /// Default constructor, creating the void type.
    Type();
    /**
     * Constructor. It is deliberately not declared @c explicit, so that
     * an instance of @ref BaseType can be used where @ref Type is expected.
     *
     * @pre @a baseType is not @c TYPE_VOID.
     */
    Type(BaseType baseType, unsigned int length = 1);

    /**
     * Whether the type can be stored in a buffer and read/written in a CL C
     * program using the assignment operator.
     */
    bool isStorable(const cl::Device &device) const;

    /**
     * Whether the type can be used in expressions.
     */
    bool isComputable(const cl::Device &device) const;
    bool isIntegral() const;       ///< True if the type stores integer values.
    bool isSigned() const;         ///< True if the type is signed.
    std::string getName() const;   ///< Name of the CL C type
    ::size_t getSize() const;      ///< Size in bytes of the C API form of the type (0 for void)
    ::size_t getBaseSize() const;  ///< Size in bytes of the scalar elements (0 for void)

    /// The scalar element type.
    BaseType getBaseType() const;
    /// The vector length (1 for scalars, 0 for void).
    unsigned int getLength() const;

    /// Returns a list of all supported types
    static std::vector<Type> allTypes();
};

namespace detail
{

/**
 * Throws an exception if a non-success error code is given.
 *
 * @param err, errStr             Error code and message (code may be @c CL_SUCCESS)
 *
 * @throw cl::Error if @a err is not @c CL_SUCCESS
 */
static inline void handleError(cl_int err, const char *errStr)
{
    if (err != CL_SUCCESS)
        throw Error(err, errStr);
}

/**
 * Creates an array of handles from a vector of wrappers. If the wrappers are
 * "thin", it just contains a pointer to the original data. Thus, the wrapped
 * data must be retained until after the wrapper is destroyed.
 */
template<typename T, bool Thin = sizeof(T) == sizeof(typename T::cl_type)>
class UnwrapArray
{
};

template<typename T>
class UnwrapArray<T, false>
{
private:
    std::vector<typename T::cl_type> raw;

public:
    explicit UnwrapArray(const VECTOR_CLASS<T> *in)
    {
        if (in != NULL)
        {
            raw.reserve(in->size());
            for (std::size_t i = 0; i < in->size(); i++)
                raw.push_back((*in)[i]());
        }
    }

    cl_uint size() const { return raw.size(); }
    const typename T::cl_type *data() const { return raw.empty() ? NULL : &raw[0]; }
};

template<typename T>
class UnwrapArray<T, true>
{
private:
    const typename T::cl_type *data_;
    std::size_t size_;

public:
    explicit UnwrapArray(const VECTOR_CLASS<T> *in)
    {
        if (in != NULL && !in->empty())
        {
            data_ = reinterpret_cast<const typename T::cl_type *>(&(*in)[0]);
            size_ = in->size();
        }
        else
        {
            data_ = NULL;
            size_ = 0;
        }
    }

    cl_uint size() const { return size_; }
    const typename T::cl_type *data() const { return data_; }
};

struct CLOGS_LOCAL CallbackWrapper
{
    void (CL_CALLBACK *callback)(const cl::Event &, void *);
    void (CL_CALLBACK *free)(void *);
    void *userData;
};

static inline void CL_CALLBACK callbackWrapperCall(cl_event event, void *userData)
{
    CallbackWrapper *wrapper = reinterpret_cast<CallbackWrapper *>(userData);
    clRetainEvent(event); // because cl::Event constructor steals a ref
    wrapper->callback(cl::Event(event), wrapper->userData);
}

static inline void CL_CALLBACK callbackWrapperFree(void *userData)
{
    CallbackWrapper *wrapper = reinterpret_cast<CallbackWrapper *>(userData);
    if (wrapper->free)
        wrapper->free(wrapper->userData);
    delete wrapper;
}

static inline CallbackWrapper *makeCallbackWrapper(
    void (CL_CALLBACK *callback)(const cl::Event &, void *),
    void *userData,
    void (CL_CALLBACK *free)(void *))
{
    CallbackWrapper *wrapper = new CallbackWrapper();
    wrapper->callback = callback;
    wrapper->userData = userData;
    wrapper->free = free;
    return wrapper;
}

template<typename T>
static inline void CL_CALLBACK genericCallbackCall(cl_event event, void *userData)
{
    T *self = reinterpret_cast<T *>(userData);
    clRetainEvent(event); // because cl::Event constructor steals a ref
    (*self)(cl::Event(event));
}

template<typename T>
static inline void CL_CALLBACK genericCallbackFree(void *userData)
{
    T *self = reinterpret_cast<T *>(userData);
    delete self;
}

class Algorithm;

} // namespace detail

/**
 * Base class for all algorithm classes.
 */
class CLOGS_API Algorithm
{
private:
    detail::Algorithm *detail_;

    /* Prevent copying */
    Algorithm(const Algorithm &) CLOGS_DELETE_FUNCTION;
    Algorithm &operator=(const Algorithm &) CLOGS_DELETE_FUNCTION;

protected:
    Algorithm();
    ~Algorithm();

    /// Constructs this by stealing the pointer from @a other
    void moveConstruct(Algorithm &other);
    /// Sets this by stealing the pointer from @a other, and returning the previous value
    detail::Algorithm *moveAssign(Algorithm &other);
    /// Swaps the pointers between this and @a other
    void swap(Algorithm &other);

    /// Returns the embedded pointer
    detail::Algorithm *getDetail() const;
    /**
     * Returns the embedded pointer.
     *
     * @throw std::logic_error if the pointer is null.
     */
    detail::Algorithm *getDetailNonNull() const;
    /**
     * Set the value of the embedded pointer. Note that this overwrites the
     * previous value without freeing it.
     */
    void setDetail(detail::Algorithm *ptr);

public:
    /**
     * Set a callback function that will receive a list of all underlying events.
     * The callback will be called multiple times during each enqueue, because
     * the implementation uses multiple commands. This allows profiling information
     * to be extracted from the events once they complete.
     *
     * The callback may also be set to @c NULL to disable it.
     *
     * @note This is not an event completion callback: it is called during
     * @c enqueue, generally before the events complete.
     *
     * @param callback The callback function.
     * @param userData Arbitrary data to be passed to the callback.
     * @param free     Passed @a userData when this object is destroyed.
     */
    void setEventCallback(
        void (CL_CALLBACK *callback)(const cl::Event &, void *),
        void *userData,
        void (CL_CALLBACK *free)(void *) = NULL)
    {
        setEventCallback(
            detail::callbackWrapperCall,
            detail::makeCallbackWrapper(callback, userData, free),
            detail::callbackWrapperFree);
    }

    /**
     * @overload
     *
     * The provided function object will be passed a cl::Event. The function
     * object type must be copyable.
     */
    template<typename T>
    void setEventCallback(const T &callback)
    {
        setEventCallback(
            detail::genericCallbackCall<T>,
            new T(callback),
            detail::genericCallbackFree<T>);
    }

    /// @overload
    void setEventCallback(
        void (CL_CALLBACK *callback)(cl_event, void *),
        void *userData,
        void (CL_CALLBACK *free)(void *) = NULL);
};

} // namespace clogs

#endif /* !CLOGS_CORE_H */
