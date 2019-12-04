/* Copyright (c) 2015, Bruce Merry
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
 * Control over tuning policy.
 */

#ifndef CLOGS_TUNE_H
#define CLOGS_TUNE_H

#include <clogs/visibility_push.h>
#include <ostream>
#include <clogs/visibility_pop.h>

namespace clogs
{

class TunePolicy;

namespace detail
{
    class TunePolicy;

    CLOGS_LOCAL const TunePolicy &getDetail(const clogs::TunePolicy &);
} // namespace detail

enum TuneVerbosity
{
    TUNE_VERBOSITY_SILENT = 0,
    TUNE_VERBOSITY_TERSE = 1,
    TUNE_VERBOSITY_NORMAL = 2,
    TUNE_VERBOSITY_DEBUG = 3
};

class CLOGS_API TunePolicy
{
private:
    friend const detail::TunePolicy &detail::getDetail(const clogs::TunePolicy &);
    detail::TunePolicy *detail_;

public:
    TunePolicy();
    ~TunePolicy();
    TunePolicy(const TunePolicy &);
    TunePolicy &operator=(const TunePolicy &);

    /**
     * Specify whether on-the-fly tuning is permitted. If it is not permitted,
     * then any attempt to construct an algorithm which isn't already tuned
     * will throw @ref clogs::CacheError. The default is that tuning is
     * permitted.
     */
    void setEnabled(bool enabled);

    /**
     * Set the verbosity level. The default is @c TUNE_VERBOSITY_NORMAL.
     */
    void setVerbosity(TuneVerbosity verbosity);

    /**
     * Set the output stream for reporting tuning progress. The default is
     * @c std::cout.
     */
    void setOutput(std::ostream &out);
};

} // namespace clogs

#endif /* !CLOGS_TUNE_H */
