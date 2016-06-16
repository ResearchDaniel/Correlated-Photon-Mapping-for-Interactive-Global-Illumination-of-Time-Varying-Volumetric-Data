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
 * Detects compiler properties.
 */

#ifndef CLOGS_PLATFORM_H
#define CLOGS_PLATFORM_H

#if __cplusplus >= 201103L
# define CLOGS_HAVE_RVALUE_REFERENCES 1
#elif defined(_MSC_VER)
# if _MSC_VER >= 1600 // VC 2010
#  define CLOGS_HAVE_RVALUE_REFERENCES 1
# endif
#elif defined(__has_extension)
# if __has_extension(cxx_rvalue_references)
#  define CLOGS_HAVE_RVALUE_REFERENCES 1
# endif
#endif

#if __cplusplus >= 201103L
# define CLOGS_HAVE_NOEXCEPT 1
#elif defined(__has_extension)
# if __has_extension(cxx_noexcept)
#  define CLOGS_HAVE_NOEXCEPT 1
# endif
#endif
#ifdef CLOGS_HAVE_NOEXCEPT
# define CLOGS_NOEXCEPT noexcept
#else
# define CLOGS_NOEXCEPT
#endif

#if __cplusplus >= 201103L
# define CLOGS_HAVE_DELETED_FUNCTIONS 1
#elif defined(_MSC_VER)
# if _MSC_VER >= 1800
#  define CLOGS_HAVE_DELETED_FUNCTIONS 1
# endif
#elif defined(__has_extension)
# if __has_extension(cxx_deleted_functions)
#  define CLOGS_HAVE_DELETED_FUNCTIONS 1
# endif
#endif
#ifdef CLOGS_HAVE_DELETED_FUNCTION
# define CLOGS_DELETE_FUNCTION = delete
#else
# define CLOGS_DELETE_FUNCTION
#endif

#endif /* !CLOGS_PLATFORM_H */
