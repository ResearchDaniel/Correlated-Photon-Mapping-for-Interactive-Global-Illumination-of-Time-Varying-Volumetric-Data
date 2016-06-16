/**
 * @file
 *
 * Includes cl.hpp with the appropriate definitions to avoid depending on any
 * OpenCL 1.2+ symbols. It must be included before anything directly includes
 * cl.hpp.
 */

#ifndef CLOGS_CLHPP11_H
#define CLOGS_CLHPP11_H

#ifdef CL_HPP_
# error "cl.hpp has already been included"
#endif

#ifndef __CL_ENABLE_EXCEPTIONS
# define __CL_ENABLE_EXCEPTIONS
#endif

#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>
#undef CL_VERSION_1_2
#undef CL_VERSION_2_0
#include <modules/opencl/cl.hpp>

#endif
