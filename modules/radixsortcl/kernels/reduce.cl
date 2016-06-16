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
 * Reduction kernel for CLOGS.
 */

#if ENABLE_KHR_FP64 && __OPENCL_C_VERSION__ <= 110
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#endif
#if ENABLE_KHR_FP16
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif

/**
 * Tests whether a value is a power of 2. This macro is suitable for use in
 * preprocessor expressions.
 * @warning Do not use with an argument that has side effects.
 */
#define IS_POWER2(x) ((x) > 0 && ((x) & ((x) - 1)) == 0)

/**
 * @def REDUCE_T
 * @hideinitializer
 * The type of data elements in the reduction.
 */

/**
 * @def REDUCE_BLOCKS
 * @hideinitializer
 * The maximum number of blocks into which the full range is subdivided. The
 * initial reduction and final summing use an array of up to @c REDUCE_BLOCKS
 * partial sums.
 */

/**
 * @def REDUCE_WORK_GROUP_SIZE
 * @hideinitializer
 * The work group size for the reduction kernel.
 */

#ifndef REDUCE_T
# error "REDUCE_T must be specified"
# define REDUCE_T int /* Keep doxygen happy */
#endif

#ifndef REDUCE_WORK_GROUP_SIZE
# error "REDUCE_WORK_GROUP_SIZE must be specified"
# define REDUCE_WORK_GROUP_SIZE 1 /* Keep doxygen happy */
#endif
#if !IS_POWER2(REDUCE_WORK_GROUP_SIZE)
# error "REDUCE_WORK_GROUP_SIZE must be a power of 2"
#endif

#ifndef REDUCE_BLOCKS
# error "REDUCE_BLOCKS is required"
# define REDUCE_BLOCKS 2 /* Keep doxygen happy */
#endif

/**
 * Shorthand for defining a kernel with a fixed work group size.
 * This is needed to unconfuse Doxygen's parser.
 */
#define KERNEL(size) __kernel __attribute__((reqd_work_group_size(size, 1, 1)))

/**
 * Work-group level reduction. The result is left in @a sums[0], which is only
 * visible to workitem 0.
 *
 * @param in       Array containing items
 * @param first    Starting position for scan
 * @param last     End position for scan
 * @param lid      Local ID of current work-item
 * @param sums     Scratch space and output area
 */
void reduceRange(
    __global const REDUCE_T * restrict in, uint first, uint last, uint lid,
    __local REDUCE_T sums[REDUCE_WORK_GROUP_SIZE])
{
    REDUCE_T accum = 0;
    for (uint i = first; i < last; i += REDUCE_WORK_GROUP_SIZE)
        if (i + lid < last)
            accum += in[i + lid];
    sums[lid] = accum;

    /* Local reduction */
    for (uint scale = REDUCE_WORK_GROUP_SIZE / 2; scale >= 1; scale >>= 1)
    {
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lid < scale)
            sums[lid] += sums[lid + scale];
    }
}

KERNEL(REDUCE_WORK_GROUP_SIZE)
void reduce(
    __global volatile uint * restrict wgc,
    __global REDUCE_T * restrict out, uint outPos,
    __global const REDUCE_T * restrict in, uint start, uint elements,
    __global REDUCE_T * restrict partial,
    uint blockSize)
{
    __local REDUCE_T sums[REDUCE_WORK_GROUP_SIZE];
    __local bool done;

    const uint group = get_group_id(0);
    const uint lid = get_local_id(0);
    const uint first = group * blockSize;
    const uint last = min(first + blockSize, elements);

    reduceRange(in + start, first, last, lid, sums);

    /* No barrier needed here, because sums[0] is computed by thread 0 */
    if (lid == 0)
    {
        partial[group] = sums[0];
        mem_fence(CLK_GLOBAL_MEM_FENCE);
        int old = atomic_dec(wgc);
        done = (old == 1);
    }

    barrier(CLK_LOCAL_MEM_FENCE); // ensures all work items see done
    if (done)
    {
        mem_fence(CLK_GLOBAL_MEM_FENCE);
        // TODO: this could be made much more efficient if wgs is bigger than blocks
        reduceRange(partial, 0, REDUCE_BLOCKS, lid, sums);
        if (lid == 0)
        {
            *wgc = REDUCE_BLOCKS;
            out[outPos] = sums[0];
        }
    }
}
