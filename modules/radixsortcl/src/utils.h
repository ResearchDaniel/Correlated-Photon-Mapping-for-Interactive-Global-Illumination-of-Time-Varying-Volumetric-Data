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
 * Utility functions that are private to the library.
 */

#ifndef UTILS_H
#define UTILS_H

#include "clhpp11.h"

#include <clogs/visibility_push.h>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <locale>
#include <stdexcept>
#include <boost/noncopyable.hpp>
#include <clogs/visibility_pop.h>

namespace clogs
{
namespace detail
{

/// Kernels source that has been embedded by clc2cpp
struct CLOGS_LOCAL Source
{
    std::string text;
    std::string checksum;

    Source() {}
    Source(const std::string &text, const std::string &checksum)
        : text(text), checksum(checksum)
    {
    }
};

/// Code shared by all the primitives
class CLOGS_LOCAL Algorithm : public boost::noncopyable
{
private:
    void (CL_CALLBACK *eventCallback)(cl_event event, void *);
    void (CL_CALLBACK *eventCallbackFree)(void *);
    void *eventCallbackUserData;

protected:
    /**
     * Call the event callback, if there is one.
     */
    void doEventCallback(const cl::Event &event);

public:
    /**
     * Set a callback to be notified of enqueued commands.
     * @see @ref clogs::Scan::setEventCallback
     */
    void setEventCallback(
        void (CL_CALLBACK *callback)(cl_event, void *),
        void *userData,
        void (CL_CALLBACK *free)(void *));

    Algorithm();
    ~Algorithm();
};

/**
 * Returns true if @a device supports @a extension.
 * At present, no caching is done, so this is a potentially slow operation.
 */
CLOGS_LOCAL bool deviceHasExtension(const cl::Device &device, const std::string &extension);

/**
 * Retrieves the kernel sources baked into the library.
 *
 * The implementation of this function is in generated code.
 */
CLOGS_LOCAL std::map<std::string, Source> &getSourceMap();

/**
 * Subgroups of this size are guaranteed to have a synchronized view of
 * local memory at sequence points, provided that memory is declared volatile.
 */
CLOGS_LOCAL unsigned int getWarpSizeMem(const cl::Device &device);

/**
 * Subgroups of this size are expected to be scheduled as SIMD, making it
 * worth avoiding branch divergence below this level. Unlike getWarpSizeMem,
 * this is purely a hint and does not affect correctness.
 */
CLOGS_LOCAL unsigned int getWarpSizeSchedule(const cl::Device &device);

/**
 * Create a context that contains only @a device.
 */
CLOGS_LOCAL cl::Context contextForDevice(const cl::Device &device);

template<typename T>
static inline std::string toString(const T &x)
{
    std::ostringstream o;
    o.imbue(std::locale::classic());
    o << x;
    return o.str();
}

/**
 * Define UNIT_TESTS when building programs. This is only for use by the
 * test code.
 */
CLOGS_LOCAL void enableUnitTests();

/**
 * Create a program. If a valid binary is found in the cache it is used,
 * otherwise the program is built from source and the cache is updated.
 */
CLOGS_LOCAL cl::Program build(
    const cl::Context &context,
    const cl::Device &device,
    const std::string &filename,
    const std::map<std::string, int> &defines,
    const std::map<std::string, std::string> &stringDefines,
    const std::string &options = "");

template<typename T>
static inline T roundDownPower2(T x)
{
    T y = 1;
    while (y * 2 <= x)
        y <<= 1;
    return y;
}

template<typename T>
static inline T roundDown(T x, T y)
{
    return x / y * y;
}

template<typename T>
static inline T roundUp(T x, T y)
{
    return (x + y - 1) / y * y;
}

static inline void checkNull(void *ptr)
{
    if (!ptr)
        throw std::logic_error("Uninitialized object");
}

/**
 * Set @a err to @c CL_SUCCESS and @a errStr to NULL.
 */
static inline void clearError(cl_int &err, const char *&errStr)
{
    err = CL_SUCCESS;
    errStr = NULL;
}

/**
 * Set @a err and @a errStr from an exception object. Either may be NULL.
 */
static inline void setError(cl_int &err, const char *&errStr, const cl::Error &exc)
{
    err = exc.err();
    errStr = exc.what();
}

/**
 * Subclasses T with a constructor that does an extra retain when constructed
 * from a base CL type.
 */
template<typename T>
class CLOGS_LOCAL RetainWrapper : public T
{
public:
    typedef typename T::cl_type cl_type;

    explicit RetainWrapper(const cl_type &obj) : T(obj)
    {
        if (obj != 0)
            this->retain();
    }
};

template<typename T>
static inline T retainWrap(typename T::cl_type obj)
{
    return RetainWrapper<T>(obj);
}

template<typename T>
static inline VECTOR_CLASS<T> retainWrap(cl_uint length, const typename T::cl_type *obj)
{
    if (length > 0 && obj == NULL)
        throw cl::Error(CL_INVALID_VALUE, "length must be zero for null arrays");
    VECTOR_CLASS<T> ret;
    ret.reserve(length);
    for (cl_uint i = 0; i < length; i++)
        ret.push_back(RetainWrapper<T>(obj[i]));
    return ret;
}

/**
 * Extract a wrapped value and steal its reference. If @a out if
 * NULL, this is a no-op.
 */
template<typename T>
static inline void unwrap(T &in, typename T::cl_type *out)
{
    if (out != NULL)
    {
        *out = in();
        in() = 0;
    }
}

} // namespace detail
} // namespace clogs

#endif /* !UTILS_H */

// windows.h defines these as macros, which break when used as std::min
#ifdef min
# undef min
#endif
#ifdef max
# undef max
#endif
