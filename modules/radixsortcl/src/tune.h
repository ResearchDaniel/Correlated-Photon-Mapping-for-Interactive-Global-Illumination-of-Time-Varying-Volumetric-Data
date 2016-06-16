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
 * Utilities for autotuning
 */

#ifndef TUNE_H
#define TUNE_H

#include <clogs/visibility_push.h>
#include <string>
#include <vector>
#include <ostream>
#include <boost/any.hpp>
#include "tr1_functional.h"
#include <clogs/visibility_pop.h>

#include <clogs/tune.h>
#include "cache_types.h"

namespace cl
{

class Context;
class Device;

} // namespace cl

namespace clogs
{
namespace detail
{

/**
 * Create a key with fields uniquely describing @a device.
 */
CLOGS_LOCAL DeviceKey deviceKey(const cl::Device &device);

class CLOGS_LOCAL TunePolicy
{
private:
    bool enabled;
    TuneVerbosity verbosity;
    std::ostream *out;

public:
    /**
     * Constructor. The default state is that tuning is permitted, verbosity level
     * is normal, and output is sent to @c std::cout.
     */
    TunePolicy();

    /// Allow or deny on-the-fly tuning
    void setEnabled(bool enabled) { this->enabled = enabled; }
    /// Set how verbose output is
    void setVerbosity(TuneVerbosity verbosity) { this->verbosity = verbosity; }
    /**
     * Set the output stream. A pointer is held, so the stream must exist until the
     * policy is destroyed.
     */
    void setOutput(std::ostream &out) { this->out = &out; }
    /// Returns whether tuning is permitted
    bool isEnabled() const { return enabled; }
    /**
     * Checks that tuning is permitted, throwing an exception if not.
     *
     * @throw clogs::CacheError if tuning is not enabled
     */
    void assertEnabled() const;

    /**
     * @name Methods called by the tuning algorithms to report progress
     * @{
     */

    /// Called at the beginning of tuning one algorithm instance
    void logStartAlgorithm(const std::string &description, const cl::Device &device) const;
    /// Called at the end of tuning one algorithm instance
    void logEndAlgorithm() const;
    /// Called at the beginning of a related set of tuning tests
    void logStartGroup() const;
    /// Called at the end of a related set of tuning tests
    void logEndGroup() const;
    /// Called at the start of a single tuning test
    void logStartTest() const;
    /**
     * Called at the end of a single tuning test.
     * @param success    Whether the test succeeded i.e. did not throw an exception
     * @param rate       Rate at which operations occurred (arbitrary scale)
     */
    void logEndTest(bool success, double rate) const;

    /**
     * @}
     */
};

/**
 * Perform low-level tuning. The callback function is called for each set of parameters,
 * and returns two values, A and B. The selected parameter set is computed as follows:
 *
 * -# The largest value of A, Amax is computed.
 * -# The first parameter set with B >= Amax is returned.
 *
 * To simply pick the
 * best, return B = A. However, if earlier parameter sets are in some
 * way intrinsicly better, then setting e.g. B = 1.05 * A will yield a
 * parameter set that has A ~= Amax but possibly much earlier. It is required
 * that A <= B.
 *
 * @a problemSizes contains values to pass to the callback. A separate phase is
 * run for each value in sequence. In the first phase, all parameter sets are
 * used. In each subsequent phase, only those whose A value was at least
 * ratio*Amax are retained. This allows for very slow parameter sets to be
 * quickly eliminated on small problem sizes (which can also avoid hardware
 * timeouts), before refining the selection on more representative problem sizes.
 *
 * It is legal for the callback function to throw @c cl::Error or @ref
 * InternalError. In either case, the parameter set will be dropped from
 * consideration.
 *
 * Each call will be made with a fresh context. It is advisable for the
 * callback function to execute a warmup pass to obtain reliable results.
 */
boost::any tuneOne(
    const TunePolicy &policy,
    const cl::Device &device,
    const std::vector<boost::any> &parameterSets,
    const std::vector<std::size_t> &problemSizes,
    FUNCTIONAL_NAMESPACE::function<
    std::pair<double, double>(
        const cl::Context &,
        const cl::Device &,
        std::size_t,
        const boost::any &)> callback,
    double ratio = 0.5);

} // namespace detail
} // namespace clogs

#endif /* TUNE_H */
