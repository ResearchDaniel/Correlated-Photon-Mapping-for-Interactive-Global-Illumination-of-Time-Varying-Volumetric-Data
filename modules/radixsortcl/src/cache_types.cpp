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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "clhpp11.h"

#include <clogs/visibility_push.h>
#include <string>
#include <vector>
#include <clogs/visibility_pop.h>

#include "parameters.h"
#include "cache_types.h"

namespace clogs
{
namespace detail
{

CLOGS_STRUCT(
    DeviceKey,
    (platformName)(deviceName)(deviceVendorId)(driverVersion)
)

CLOGS_STRUCT(
    KernelParameters::Key,
    (device)(header)(checksum)
)
CLOGS_STRUCT(
    KernelParameters::Value,
    (binary)
)

CLOGS_STRUCT(
    ScanParameters::Key,
    (device)
    (elementType)
)
CLOGS_STRUCT(
    ScanParameters::Value,
    (warpSizeMem)
    (warpSizeSchedule)
    (reduceWorkGroupSize)
    (scanWorkGroupSize)
    (scanWorkScale)
    (scanBlocks)
)

CLOGS_STRUCT(
    ReduceParameters::Key,
    (device)
    (elementType)
)
CLOGS_STRUCT(
    ReduceParameters::Value,
    (reduceWorkGroupSize)
    (reduceBlocks)
)

CLOGS_STRUCT(
    RadixsortParameters::Key,
    (device)
    (keyType)
    (valueSize)
)
CLOGS_STRUCT(
    RadixsortParameters::Value,
    (warpSizeMem)
    (warpSizeSchedule)
    (reduceWorkGroupSize)
    (scanWorkGroupSize)
    (scatterWorkGroupSize)
    (scatterWorkScale)
    (scanBlocks)
    (radixBits)
)

CLOGS_LOCAL DeviceKey deviceKey(const cl::Device &device)
{
    DeviceKey key;
    cl::Platform platform(device.getInfo<CL_DEVICE_PLATFORM>());
    key.platformName = platform.getInfo<CL_PLATFORM_NAME>();
    key.deviceName = device.getInfo<CL_DEVICE_NAME>();
    key.deviceVendorId = device.getInfo<CL_DEVICE_VENDOR_ID>();
    key.driverVersion = device.getInfo<CL_DRIVER_VERSION>();
    return key;
}

} // namespace detail
} // namespace clogs
