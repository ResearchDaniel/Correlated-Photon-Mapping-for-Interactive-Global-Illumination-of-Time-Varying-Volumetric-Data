/* Copyright (c) 2014 Bruce Merry
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
 * Structures used in the persistent cache.
 */

#ifndef CACHE_TYPES_H
#define CACHE_TYPES_H

#include "clhpp11.h"

#include <clogs/visibility_push.h>
#include <string>
#include <vector>
#include <clogs/visibility_pop.h>

#include "parameters.h"

namespace clogs
{
namespace detail
{

/// Lookup key for an OpenCL device, for tuning caching
struct CLOGS_LOCAL DeviceKey
{
    std::string platformName;   ///< @c CL_PLATFORM_NAME
    std::string deviceName;     ///< @c CL_DEVICE_NAME
    cl_uint deviceVendorId;     ///< @c CL_DEVICE_VENDOR_ID
    std::string driverVersion;  ///< @c CL_DRIVER_VERSION
};

CLOGS_STRUCT_FORWARD(DeviceKey)

class CLOGS_LOCAL KernelParameters
{
public:
    struct CLOGS_LOCAL Key
    {
        DeviceKey device;
        std::string header;
        std::string checksum;
    };

    struct CLOGS_LOCAL Value
    {
        std::vector<unsigned char> binary;
    };

    static const char *tableName() { return "kernel_v1"; }
};

CLOGS_STRUCT_FORWARD(KernelParameters::Key)
CLOGS_STRUCT_FORWARD(KernelParameters::Value)

class CLOGS_LOCAL ScanParameters
{
public:
    struct CLOGS_LOCAL Key
    {
        DeviceKey device;
        std::string elementType;
    };

    struct CLOGS_LOCAL Value
    {
        ::size_t warpSizeMem;
        ::size_t warpSizeSchedule;
        ::size_t reduceWorkGroupSize;
        ::size_t scanWorkGroupSize;
        ::size_t scanWorkScale;
        ::size_t scanBlocks;
    };

    static const char *tableName() { return "scan_v6"; }
};

CLOGS_STRUCT_FORWARD(ScanParameters::Key)
CLOGS_STRUCT_FORWARD(ScanParameters::Value)

class CLOGS_LOCAL ReduceParameters
{
public:
    struct CLOGS_LOCAL Key
    {
        DeviceKey device;
        std::string elementType;
    };

    struct CLOGS_LOCAL Value
    {
        ::size_t reduceWorkGroupSize;
        ::size_t reduceBlocks;
    };

    static const char *tableName() { return "reduce_v1"; }
};

CLOGS_STRUCT_FORWARD(ReduceParameters::Key)
CLOGS_STRUCT_FORWARD(ReduceParameters::Value)

class CLOGS_LOCAL RadixsortParameters
{
public:
    struct Key
    {
        DeviceKey device;
        std::string keyType;
        ::size_t valueSize;
    };

    struct Value
    {
        ::size_t warpSizeMem;
        ::size_t warpSizeSchedule;
        ::size_t reduceWorkGroupSize;
        ::size_t scanWorkGroupSize;
        ::size_t scatterWorkGroupSize;
        ::size_t scatterWorkScale;
        ::size_t scanBlocks;
        unsigned int radixBits;
    };

    static const char *tableName() { return "radixsort_v5"; }
};

CLOGS_STRUCT_FORWARD(RadixsortParameters::Key)
CLOGS_STRUCT_FORWARD(RadixsortParameters::Value)

/**
 * Create a key with fields uniquely describing @a device.
 */
CLOGS_LOCAL DeviceKey deviceKey(const cl::Device &device);

} // namespace detail
} // namespace clogs

#endif /* CACHE_TYPES_H */
