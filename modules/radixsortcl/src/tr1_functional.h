/* Copyright (c) 2012 University of Cape Town
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
 * Include the functionality of <tr1/functional> from an appropriate place.
 * It doesn't work to just include <boost/tr1/functional.hpp>, because under GCC
 * this conflicts with the system <tr1/functional> that is pulled in by cl.hpp.
 */

#ifndef CLOGS_TR1_FUNCTIONAL_H
#define CLOGS_TR1_FUNCTIONAL_H

#if HAVE_CONFIG_H
# include <config.h>
#endif
#if HAVE_TR1_FUNCTIONAL
# include <tr1/functional>
# define FUNCTIONAL_NAMESPACE std::tr1
#elif HAVE_FUNCTIONAL
# include <functional>
# define FUNCTIONAL_NAMESPACE std
#else
# include <boost/tr1/functional.hpp>
# define FUNCTIONAL_NAMESPACE std::tr1
#endif

#endif /* CLOGS_TR1_FUNCTIONAL_H */
