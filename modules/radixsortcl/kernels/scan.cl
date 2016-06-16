/* Copyright (c) 2012, 2014 University of Cape Town
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
 * Scan kernels for CLOGS.
 */

/**
 * Tests whether a value is a power of 2. This macro is suitable for use in
 * preprocessor expressions.
 * @warning Do not use with an argument that has side effects.
 */
#define IS_POWER2(x) ((x) > 0 && ((x) & ((x) - 1)) == 0)

/**
 * @def SCAN_T
 * @hideinitializer
 * The type of data elements in the scan.
 */

/**
 * @def SCAN_PAD_T
 * @hideinitializer
 * If @ref SCAN_T is a 3-element type, this is the corresponding 4-element
 * type. It is used to work around driver bugs that make 3-element kernel
 * argument break. In other cases, it is undefined.
 */

/**
 * @def WARP_SIZE_MEM
 * @hideinitializer
 * The granularity at which a barrier can be omitted for communication using
 * local memory. This can safely be a factor of the true answer for the
 * hardware.
 */

/**
 * @def WARP_SIZE_SCHEDULE
 * @hideinitializer
 * A hint for the number of threads that run in lockstep and which should
 * avoid divergent branching.
 */

/**
 * @def REDUCE_WORK_GROUP_SIZE
 * @hideinitializer
 * The work group size for the initial reduction kernel in a scan operation.
 */

/**
 * @def SCAN_BLOCKS
 * @hideinitializer
 * The maximum number of blocks into which the full range is subdivided. The
 * initial reduction and final scan use an array of up to @c SCAN_BLOCKS
 * partial sums.
 */

/**
 * @def SCAN_WORK_GROUP_SIZE
 * @hideinitializer
 * The work group size for the final scan kernel in a scan operation.
 */

/**
 * @def SCAN_WORK_SCALE
 * @hideinitializer
 * The number of elements to process per thread in the final scan kernel.
 */

#ifndef SCAN_T
# error "SCAN_T must be specified"
# define SCAN_T int /* Keep doxygen happy */
#endif

#ifndef SCAN_PAD_T
# define SCAN_PAD_T SCAN_T
# define SCAN_UNPAD(x) (x)
#else
# define SCAN_UNPAD(x) ((x).s012)
#endif

#ifndef WARP_SIZE_MEM
# error "WARP_SIZE_MEM must be specified"
# define WARP_SIZE_MEM 1 /* Keep doxygen happy */
#endif
#if !IS_POWER2(WARP_SIZE_MEM)
# error "WARP_SIZE_MEM must be a power of 2"
#endif

#ifndef WARP_SIZE_SCHEDULE
# error "WARP_SIZE_SCHEDULE must be specified"
# define WARP_SIZE_SCHEDULE 1 /* Keep doxygen happy */
#endif
#if !IS_POWER2(WARP_SIZE_SCHEDULE)
# error "WARP_SIZE_SCHEDULE must be a power of 2"
#endif

#ifndef REDUCE_WORK_GROUP_SIZE
# error "REDUCE_WORK_GROUP_SIZE must be specified"
# define REDUCE_WORK_GROUP_SIZE 1 /* Keep doxygen happy */
#endif
#if !IS_POWER2(REDUCE_WORK_GROUP_SIZE)
# error "REDUCE_WORK_GROUP_SIZE must be a power of 2"
#endif

#ifndef SCAN_WORK_GROUP_SIZE
# error "SCAN_WORK_GROUP_SIZE must be specified"
# define SCAN_WORK_GROUP_SIZE 1 /* Keep doxygen happy */
#endif
#if !IS_POWER2(SCAN_WORK_GROUP_SIZE)
# error "SCAN_WORK_GROUP_SIZE must be a power of 2"
#endif

#ifndef SCAN_BLOCKS
# error "SCAN_BLOCKS is required"
# define SCAN_BLOCKS 2 /* Keep doxygen happy */
#endif
#if SCAN_BLOCKS & 1
# error "SCAN_BLOCKS must be even"
#endif

#ifndef SCAN_WORK_SCALE
# error "SCAN_WORK_SCALE must be specified"
# define SCAN_WORK_SCALE 1 /* Keep doxygen happy */
#endif
#if !IS_POWER2(SCAN_WORK_SCALE)
# error "SCAN_WORK_SCALE must be a power of 2"
#endif

/**
 * Shorthand for defining a kernel with a fixed work group size.
 * This is needed to unconfuse Doxygen's parser.
 */
#define KERNEL(size) __kernel __attribute__((reqd_work_group_size(size, 1, 1)))

/**
 * Compute sums of contiguous ranges of elements.
 * @param out    Reduced output values.
 * @param in     Input values to reduce.
 * @param len    Number of values to reduce per work-group
 *
 * @pre @a len is a multiple of @ref REDUCE_WORK_GROUP_SIZE
 * @todo Skip barriers and conditions below @ref WARP_SIZE_MEM.
 */
KERNEL(REDUCE_WORK_GROUP_SIZE)
void reduce(__global SCAN_T *out, __global const SCAN_T *in, uint len)
{
    __local SCAN_T sums[REDUCE_WORK_GROUP_SIZE];
    const uint group = get_group_id(0);
    const uint lid = get_local_id(0);
    const uint in_offset = group * len + lid;

    /* Sum up corresponding elements from each chunk */
    SCAN_T accum = 0;
    for (uint i = 0; i < len; i += REDUCE_WORK_GROUP_SIZE)
         accum += in[in_offset + i];
    sums[lid] = accum;

    /* Upsweep */
    for (uint scale = REDUCE_WORK_GROUP_SIZE / 2; scale >= 1; scale >>= 1)
    {
        barrier(CLK_LOCAL_MEM_FENCE);
        if (lid < scale)
            sums[lid] += sums[lid + scale];
    }

    /* No barrier needed here, because sums[0] is computed by thread 0 */
    if (lid == 0)
        out[group] = sums[0];
}

// v has size SCAN_BLOCKS
inline void scanExclusiveSmallBottom(__local SCAN_T *v, uint lid)
{
    /* Upsweep */
    uint pos = lid + 1;
    uint scale;
    for (scale = 1; scale <= SCAN_BLOCKS / 2; scale <<= 1)
    {
        pos <<= 1;
        if (pos <= SCAN_BLOCKS)
            v[pos - 1] += v[pos - scale - 1];
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    scale >>= 1; // undo the last scale <<= 1 at the end of the loop

    /* Downsweep */
    for (; scale >= 1; scale >>= 1)
    {
        if (pos <= SCAN_BLOCKS - scale)
            v[pos + scale - 1] += v[pos - 1];
        barrier(CLK_LOCAL_MEM_FENCE);
        pos >>= 1;
    }

}

/**
 * Does an exclusive prefix sum on @ref SCAN_BLOCKS elements.
 *
 * @param inout  The values to scan, replaced with result.
 * @param offset Offset to add to all elements.
 *
 * @pre @ref SCAN_BLOCKS is even
 * @todo skip barriers and conditions below @ref WARP_SIZE_MEM.
 */
KERNEL(SCAN_BLOCKS / 2)
void scanExclusiveSmall(__global SCAN_T *inout, SCAN_PAD_T offset)
{
    const unsigned int lid = get_local_id(0);
    const unsigned int wgs = SCAN_BLOCKS / 2; // work group size
    __local SCAN_T v[SCAN_BLOCKS];

    /* Copy to local memory for computation, shifting by one to turn an
     * exclusive problem into an inclusive one
     */
    v[lid] = (lid == 0) ? SCAN_UNPAD(offset) : inout[lid - 1];
    v[lid + wgs] = inout[lid + wgs - 1];
    barrier(CLK_LOCAL_MEM_FENCE);

    scanExclusiveSmallBottom(v, lid);

    /* Writeback */
    inout[lid] = v[lid];
    inout[lid + wgs] = v[lid + wgs];
}

/**
 * Does an exclusive prefix sum on @ref SCAN_BLOCKS elements, with an
 * offset encoded in a buffer.
 *
 * @param[in,out] inout        The values to scan, replaced with result.
 * @param offset               A fixed offset to add to all values.
 * @param offsetIndex          Index (in units of SCAN_T) into @a offset to find the offset.
 *
 * @pre @ref SCAN_BLOCKS is even
 * @todo skip barriers and conditions below @ref WARP_SIZE.
 */
KERNEL(SCAN_BLOCKS / 2)
void scanExclusiveSmallOffset(__global SCAN_T *inout,
                              __global const SCAN_T *offset,
                              uint offsetIndex)
{
    const unsigned int lid = get_local_id(0);
    const unsigned int wgs = SCAN_BLOCKS / 2; // work group size
    __local SCAN_T v[SCAN_BLOCKS];

    /* Copy to local memory for computation, shifting by one to turn an
     * exclusive problem into an inclusive one
     */
    v[lid] = (lid == 0) ? offset[offsetIndex] : inout[lid - 1];
    v[lid + wgs] = inout[lid + wgs - 1];
    barrier(CLK_LOCAL_MEM_FENCE);

    scanExclusiveSmallBottom(v, lid);

    /* Writeback */
    inout[lid] = v[lid];
    inout[lid + wgs] = v[lid + wgs];
}

/**
 * Does an exclusive scan a possibly large range, given initial offsets per work-group.
 *
 * @param[in]     in      Sequence to scan
 * @param[out]    out     Prefix sums (may be the same buffer as @a in
 * @param         offsets The starting offset for each work-group
 * @param         len     Number of elements to scan per work-group
 * @param         total   Total number of elements to scan
 *
 * @pre @a len is a multiple of @c SCAN_WORK_SCALE * @c SCAN_WORK_GROUP_SIZE
 */
KERNEL(SCAN_WORK_GROUP_SIZE)
void scanExclusive(
    __global const SCAN_T *in,
    __global SCAN_T *out,
    __global const SCAN_T *offsets,
    uint len,
    uint total)
{
    /* The algorithm operates on tiles of size SCAN_WORK_GROUP_SIZE*SCAN_WORK_SCALE.
     * Each tile is scanned in a two-level hierarchy. Each workitem reads SCAN_WORK_SCALE
     * contiguous elements, which it manages in private memory (they temporarily sit in
     * shared memory, but only to manage global memory access patterns). The workitem
     * - does a local scan on these elements
     * - passes the sum up to the next level of the hierarchy, in shared memory.
     * Then a multiresolution scan is done. To avoid bank conflicts, the intermediate
     * results of each layer are held separately in memory, rather than overwritten in
     * place.
     *
     * Finally, the coarse scan results are transferred back to the per-thread data to
     * adjust them, before they're written back to global memory.
     */
    __local union
    {
        /* Used as a staging area for load and store, to allow memory
         * transactions to be contiguous.
         */
        SCAN_T raw[SCAN_WORK_GROUP_SIZE * SCAN_WORK_SCALE];

        /* At the end of upsweep, [ 2^n, 2^(n+1) ) contains a reduced form of the
         * full sequence. Downsweep turns each such range into its exclusive
         * prefix sum.
         */
        SCAN_T reduced[2 * SCAN_WORK_GROUP_SIZE];
    } x;
    SCAN_T priv[SCAN_WORK_SCALE];

    const uint lid = get_local_id(0);
    SCAN_T offset;

    size_t bias = get_group_id(0) * len;
    in += bias;
    out += bias;
    total -= bias;
    offset = offsets[get_group_id(0)];
    for (uint start = 0; start < len; start += SCAN_WORK_SCALE * SCAN_WORK_GROUP_SIZE)
    {
        /* Load the raw data using coalesced reads */
        for (uint i = 0; i < SCAN_WORK_SCALE; i++)
        {
            uint addr = start + lid + i * SCAN_WORK_GROUP_SIZE;
            x.raw[lid + i * SCAN_WORK_GROUP_SIZE] = (addr < total) ? in[addr] : (SCAN_T) 0;
        }
        barrier(CLK_LOCAL_MEM_FENCE);

        /* Read the relevant data into registers */
        for (uint i = 0; i < SCAN_WORK_SCALE; i++)
        {
            priv[i] = x.raw[lid * SCAN_WORK_SCALE + i];
        }

        /* Scan the private range */
        for (uint i = 0; i < SCAN_WORK_SCALE - 1; i++)
            priv[i + 1] += priv[i];

        /* Write the reduced private ranges for shared upsweep */
        barrier(CLK_LOCAL_MEM_FENCE);
        x.reduced[SCAN_WORK_GROUP_SIZE + lid] = priv[SCAN_WORK_SCALE - 1];
        barrier(CLK_LOCAL_MEM_FENCE);

        /* Upsweep, interwarp */
        for (uint scale = SCAN_WORK_GROUP_SIZE / 2; scale >= 1; scale >>= 1)
        {
            if (lid < scale)
            {
                const uint pos = scale + lid;
                x.reduced[pos] = x.reduced[2 * pos] + x.reduced[2 * pos + 1];
            }
            if (scale > WARP_SIZE_MEM)
                barrier(CLK_LOCAL_MEM_FENCE);
            else
                mem_fence(CLK_LOCAL_MEM_FENCE); // TODO: replace all mem_fence with volatile
        }

        /* v[1] is the total of this range, but need to make it exclusive */
        if (lid == 0)
        {
            SCAN_T nextOffset = offset + x.reduced[1];
            x.reduced[1] = offset;
            offset = nextOffset;
        }
        /* No barrier needed here, because only thread 0 uses x.reduced[1] */

        /* Downsweep */
        for (uint scale = 1; scale < SCAN_WORK_GROUP_SIZE; scale <<= 1)
        {
            if (lid < scale)
            {
                const uint pos = scale + lid;
                const SCAN_T in = x.reduced[pos];
                const SCAN_T left = x.reduced[2 * pos];
                x.reduced[2 * pos + 1] = in + left;
                x.reduced[2 * pos] = in;
            }
            if (scale >= WARP_SIZE_MEM)
                barrier(CLK_LOCAL_MEM_FENCE);
            else
                mem_fence(CLK_LOCAL_MEM_FENCE);
        }

        /* Feed reduction back into private range, making it exclusive at the same time */
        const SCAN_T add = x.reduced[SCAN_WORK_GROUP_SIZE + lid];
        barrier(CLK_LOCAL_MEM_FENCE);
        for (uint i = SCAN_WORK_SCALE - 1; i > 0; i--)
        {
            x.raw[lid * SCAN_WORK_SCALE + i] = priv[i - 1] + add;
        }
        x.raw[lid * SCAN_WORK_SCALE] = add;
        barrier(CLK_LOCAL_MEM_FENCE);

        /* Writeback */
        for (uint i = 0; i < SCAN_WORK_SCALE; i++)
        {
            uint addr = start + lid + i * SCAN_WORK_GROUP_SIZE;
            if (addr < total)
                out[addr] = x.raw[lid + i * SCAN_WORK_GROUP_SIZE];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}
