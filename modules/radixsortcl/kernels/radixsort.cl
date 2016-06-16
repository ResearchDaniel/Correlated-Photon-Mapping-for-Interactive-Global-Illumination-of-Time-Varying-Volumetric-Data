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
 * Kernels for radix sorting.
 */

/**
 * Tests whether a value is a power of 2. This macro is suitable for use in
 * preprocessor expressions.
 * @warning Do not use with an argument that has side effects.
 */
#define IS_POWER2(x) ((x) > 0 && ((x) & ((x) - 1)) == 0)

/**
 * Maximum of two values, usable as a preprocessor expression.
 */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @def KEY_T
 * @hideinitializer
 * The type of the keys.
 */

/**
 * @def VALUE_T
 * @hideinitializer
 * The type of the values.
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
 * The work group size for the initial reduction kernel.
 */

/**
 * @def SCAN_WORK_GROUP_SIZE
 * @hideinitializer
 * The work group size for the middle scan kernel.
 */

/**
 * @def SCAN_BLOCKS
 * @hideinitializer
 * The maximum number of blocks into which the full range is subdivided. The
 * initial reduction and final scatter use an array of up to @c SCAN_BLOCKS
 * partial sums per radix.
 */

/**
 * @def SCATTER_WORK_GROUP_SIZE
 * @hideinitializer
 * The work group size for the final scatter kernel.
 */

/**
 * @def SCATTER_WORK_SCALE
 * @hideinitializer
 * The number of elements to process per workitem in the final scatter kernel.
 */

/**
 * @def SCATTER_SLICE
 * @hideinitializer
 * The number of workitems that the scatter kernel uses in cooperation with
 * each other.
 */

/**
 * @def RADIX_BITS
 * @hideinitializer
 * The number of bits to extract from the key in each sorting pass.
 */

/**
 * @def UPSWEEP
 * @hideinitializer
 * Generated code for reducing the @c level1 array. It must yield digit
 * counts.
 */

/**
 * @def DOWNSWEEP
 * @hideinitializer
 * Generated code for prefix summing the @c level1 array, which runs
 * after @ref UPSWEEP.
 */

#ifndef KEY_T
# error "KEY_T must be specified"
# define KEY_T uint /* Keep doxygen happy */
#endif

#ifndef RADIX_BITS
# error "RADIX_BITS must be specified"
# define RADIX_BITS 1 /* Keep doxygen happy */
#endif

/// The sort radix
#define RADIX (1U << (RADIX_BITS))

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
#if REDUCE_WORK_GROUP_SIZE < RADIX
# error "REDUCE_WORK_GROUP_SIZE must be at least RADIX"
#endif

#ifndef SCAN_BLOCKS
# error "SCAN_BLOCKS is required"
# define SCAN_BLOCKS 2 /* Keep doxygen happy */
#endif
#ifndef SCAN_WORK_GROUP_SIZE
# error "SCAN_WORK_GROUP_SIZE is required"
# define SCAN_WORK_GROUP_SIZE 1 /* Keep doxygen happy */
#endif
#if !IS_POWER2(SCAN_WORK_GROUP_SIZE)
# error "SCAN_WORK_GROUP_SIZE must be a power of 2"
#endif
#if SCAN_BLOCKS * RADIX % SCAN_WORK_GROUP_SIZE != 0
# error "SCAN_WORK_GROUP_SIZE must divide into SCAN_BLOCKS * RADIX"
#endif
#if SCAN_WORK_GROUP_SIZE < RADIX
# error "SCAN_WORK_GROUP_SIZE must be at least RADIX"
#endif

#ifndef UPSWEEP
# error "UPSWEEP must be defined"
# define UPSWEEP() do {} while (0)   /* Keep doxygen happy */
#endif
#ifndef DOWNSWEEP
# error "DOWNSWEEP must be defined"
# define DOWNSWEEP() do {} while (0) /* Keep doxygen happy */
#endif

#ifndef SCATTER_WORK_SCALE
# error "SCATTER_WORK_SCALE must be specified"
# define SCATTER_WORK_SCALE 1 /* Keep doxygen happy */
#endif
#ifndef SCATTER_WORK_GROUP_SIZE
# error "SCATTER_WORK_GROUP_SIZE must be specified"
# define SCATTER_WORK_GROUP_SIZE 1 /* Keep doxygen happy */
#endif
#if !IS_POWER2(SCATTER_WORK_GROUP_SIZE)
# error "SCATTER_WORK_GROUP_SIZE must be a power of 2"
#endif
#if RADIX < 4
# error "RADIX must be at least 4"
#endif
#ifndef SCATTER_SLICE
# error "SCATTER_SLICE must be specified"
# define SCATTER_SLICE 1 /* Keep doxygen happy */
#endif
#if !IS_POWER2(SCATTER_SLICE)
# error "SCATTER_SLICE must be a power of 2"
#endif
#if SCATTER_WORK_GROUP_SIZE % SCATTER_SLICE != 0
# error "SCATTER_WORK_GROUP_SIZE must be a multiple of SCATTER_SLICE"
#endif
#if SCATTER_SLICE < RADIX
# error "SCATTER_SLICE must be at least RADIX"
#endif
#if SCATTER_SLICE * SCATTER_WORK_SCALE >= 256
# error "SCATTER_SLICE * SCATTER_WORK_GROUP_SCALE must be strictly less than 256"
#endif

#if WARP_SIZE_MEM > 1
# define WARP_VOLATILE volatile
#else
# define WARP_VOLATILE
#endif

/**
 * Shorthand for defining a kernel with a fixed work group size.
 * This is needed to unconfuse Doxygen's parser.
 */
#define KERNEL(size) __kernel __attribute__((reqd_work_group_size(size, 1, 1)))

/**
 * Extract keys and compute histograms for a range.
 * For each of @a len keys, extracts the @ref RADIX_BITS bits starting from
 * @a firstBit to determine a bucket. These are summed to give a histogram,
 * which is written out to <code>out + RADIX * groupid</code>.
 *
 * @pre @a len is a multiple of @c REDUCE_WORK_GROUP_SIZE
 * @todo Take advantage of @c WARP_SIZE_MEM and/or @c WARP_SIZE_SCHEDULE
 * @todo Rewrite using slices (as for scatter)
 * @todo Rewrite using @c uchar for per-tile counts
 */
KERNEL(REDUCE_WORK_GROUP_SIZE)
void radixsortReduce(__global uint *out, __global const KEY_T *keys,
                     uint len, uint total, uint firstBit)
{
    const uint lid = get_local_id(0);
    const uint group = get_group_id(0);
    const uint base = group * len;
    const uint end = min(base + len, total);
    out += group * RADIX;

    /* Per-radix counts. Initially they are per-workitem, which are then
     * reduced to single counts.
     */
    __local uint hist[RADIX][REDUCE_WORK_GROUP_SIZE];

    /* Zero out hist */
    for (uint i = 0; i < RADIX; i++)
    {
        hist[i][lid] = 0;
    }

    /* Accumulate all chunks into the histogram */
    for (uint i = base + lid; i < end; i += REDUCE_WORK_GROUP_SIZE)
    {
        const KEY_T key = keys[i];
        const uint bucket = (key >> firstBit) & (RADIX - 1);
        hist[bucket][lid]++;
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    const uint ratio = REDUCE_WORK_GROUP_SIZE / RADIX;
    const uint digit = lid / ratio;
    const uint c = lid & (ratio - 1);

    uint sum = 0;
    for (uint i = 0; i < RADIX; i++)
    {
        sum += hist[digit][i * ratio + c];
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    hist[digit][c] = sum;
    barrier(CLK_LOCAL_MEM_FENCE);

    /* Reduce all histograms in parallel.
     */
#pragma unroll
    for (uint scale = ratio / 2; scale >= 1; scale >>= 1)
    {
        if (c < scale)
        {
            sum += hist[digit][c + scale];
            hist[digit][c] = sum;
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    /* Write back results, which are in hist[?][0]. Note: this currently causes a
     * total bank conflict, but padding the array would cause conflicts during the
     * more expensive accumulation phase.
     */
    if (lid < RADIX)
        out[lid] = hist[lid][0];
}

/**
 * Column-wise exclusive scan of histograms at top level.
 *
 * @param[in,out] histogram       The per-block histograms, with @ref RADIX counts per block.
 * @param         blocks          Number of blocks to scan
 *
 * @pre @a blocks <= @c SCAN_BLOCKS
 * @note @a histogram must have space allocated for @c SCAN_BLOCKS blocks even if fewer are
 * used, and the remaining space has undefined values on return.
 */
KERNEL(SCAN_WORK_GROUP_SIZE)
void radixsortScan(__global uint *histogram, uint blocks)
{
    __local uint hist[SCAN_BLOCKS * RADIX];
    __local uint sums[2 * SCAN_WORK_GROUP_SIZE];

    const uint lid = get_local_id(0);
    /* Load the data from global memory */
    uint limit = blocks * RADIX;
    for (uint i = 0; i < SCAN_BLOCKS * RADIX; i += SCAN_WORK_GROUP_SIZE)
    {
        uint addr = i + lid;
        uint v = histogram[addr];
        hist[addr] = (addr < limit) ? v : 0;
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    /* Use entire workgroup to do local prefix sums on chunks of size
     * chunkRows.
     */
    const uint chunks = SCAN_WORK_GROUP_SIZE / RADIX;
    const uint chunkRows = SCAN_BLOCKS / chunks;
    const uint digit = lid % RADIX;
    const uint chunk = lid / RADIX;
    const uint firstRow = chunk * chunkRows;

    uint sum = 0;
    for (uint i = 0; i < chunkRows; i++)
    {
        uint addr = (firstRow + i) * RADIX + digit;
        uint next = hist[addr];
        hist[addr] = sum;
        sum += next;
    }

    // Save and transpose
    sums[lid] = 0;
    sums[SCAN_WORK_GROUP_SIZE + digit * chunks + chunk] = sum;

    barrier(CLK_LOCAL_MEM_FENCE);

    // Prefix sum the sums array
    sum = sums[SCAN_WORK_GROUP_SIZE + lid];
    for (uint scale = 1; scale <= SCAN_WORK_GROUP_SIZE / 2; scale *= 2)
    {
        uint prev = sums[SCAN_WORK_GROUP_SIZE - scale + lid];
        barrier(CLK_LOCAL_MEM_FENCE);
        sum += prev;
        sums[SCAN_WORK_GROUP_SIZE + lid] = sum;
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    /* Transfer prefix-summed sums back to individual entries, and at the
     * same time write them out.
     */
    for (uint i = 0; i < SCAN_BLOCKS * RADIX; i += SCAN_WORK_GROUP_SIZE)
    {
        const uint chunk = (i + lid) / (chunkRows * RADIX);
        sum = sums[SCAN_WORK_GROUP_SIZE - 1 + digit * chunks + chunk];
        const uint total = hist[i + lid] + sum;
        histogram[i + lid] = total;
    }
}

/**
 * Number of keys processed by each iteration of each slice.
 */
#define SCATTER_TILE (SCATTER_SLICE * SCATTER_WORK_SCALE)
/**
 * Number of slices per workgroup in scatter kernel.
 */
#define SCATTER_SLICES (SCATTER_WORK_GROUP_SIZE / SCATTER_SLICE)

/**
 * Synchronize between groups of @a threads workitems.
 * Unlike calling @c barrier directly, this function is fast if @a threads
 * is less than @ref WARP_SIZE_MEM.
 *
 * @pre @a threads is a power of 2.
 */
inline void fastsync(uint threads)
{
    if (threads > WARP_SIZE_MEM)
        barrier(CLK_LOCAL_MEM_FENCE);
}

/**
 * Add the 4 bytes packed into a 32-bit word.
 */
inline uint sum4(uint v)
{
    /* 0xaabbccdd * 0x01010101 == 0x(a+b+c+d)(b+c+d)(c+d)(d), so
     * taking the high byte gives the sum. This works on any
     * endian system.
     */
    return (v * 0x01010101) >> 24;
}

/**
 * Add the 2 bytes packed into a 16-bit word.
 */
inline uint sum2(uint v)
{
    // See sum4
    return (v * 0x0101) >> 8;
}

/**
 * Compute sums of groups of elements.
 * The input data is a sequence of @a fullSize unsigned chars. Each group
 * of @a fullSize / @a sumsSize elements is added and written back to @a
 * sums.
 *
 * The function must be called by exactly @a sumsSize workitems, which will
 * cooperatively compute the sums.
 *
 * @param[in]    data          The sequence, aliased as an array of uints.
 * @param[out]   sums          The sums.
 * @param        fullSize      Length of the input sequence.
 * @param        sumsSize      Length of the output sequence.
 * @param        lid           ID for this calling workitem.
 *
 * @pre
 * - Suitable barriers are in place for all the calling workitems to see all the data.
 * - @a sumsSize is a power of 2.
 * - @a lid takes on the values 0, 1, ..., @a sumsSize - 1 across the calling workitems.
 * - The sums do not overflow.
 * @post
 * - All workitems that participated will have visibility of all the results.
 */
inline void upsweepMulti(__local const WARP_VOLATILE uint * restrict data,
                         __local WARP_VOLATILE uchar * restrict sums,
                         uint fullSize, uint sumsSize, uint lid)
{
    const uint rounds = fullSize / sumsSize / 4;
    uint sum = 0;
#pragma unroll
    for (uint i = 0; i < rounds; i++)
        sum += data[lid * rounds + i];
    sums[lid] = sum4(sum);
    fastsync(sumsSize);
}

/**
 * Compute sums of sets of 4 elements.
 * The input data is a sequence of 4 * @a sumsSize unsigned chars. Each group
 * of 4 contiguous elements is added and written back to @a sums.
 *
 * The function must be called by some number of workitems, which will
 * cooperatively compute the sums. Ideally this should not be more than @ref
 * WARP_SIZE_SCHEDULE if fewer workitems are needed, since if there are too
 * many then the extras will repeat the work rather than branching around it.
 *
 * @param[in]    data          The sequence, aliased as an array of uints.
 * @param[out]   sums          The sums.
 * @param        sumsSize      Length of the output sequence.
 * @param        lid           ID for this calling workitem.
 * @param        threads       Number of workitems that are calling this function.
 *
 * @pre
 * - Suitable barriers are in place for all the calling workitems to see all the data.
 * - @a sumsSize is a power of 2.
 * - @a threads is a power of 2 which is at least @a sumsSize.
 * - @a lid takes on the values 0, 1, ..., @a threads - 1 across the calling workitems.
 * - The sums do not overflow.
 * @post
 * - All workitems that participated will have visibility of all the results.
 */
inline void upsweep4(__local const WARP_VOLATILE uint * restrict data, __local WARP_VOLATILE uchar * restrict sums,
                     uint sumsSize, uint lid, uint threads)
{
    if (sumsSize < threads)
        lid &= sumsSize - 1;         // too many workitems - repeat work

    uint in = data[lid];
    sums[lid] = sum4(in);

    fastsync(threads); // TODO: could be a smaller number?
}

/**
 * Compute sums of sets of 2 elements.
 * The input data is a sequence of 2 * @a sumsSize unsigned chars. Each group
 * of 2 contiguous elements is added and written back to @a sums.
 *
 * The function must be called by some number of workitems, which will
 * cooperatively compute the sums. Ideally this should not be more than @ref
 * WARP_SIZE_SCHEDULE if fewer workitems are needed, since if there are too
 * many then the extras will repeat the work rather than branching around it.
 *
 * @param[in]    data          The sequence, aliased as an array of ushorts.
 * @param[out]   sums          The sums.
 * @param        sumsSize      Length of the output sequence.
 * @param        lid           ID for this calling thread
 * @param        threads       Number of workitems that are calling this function.
 *
 * @pre
 * - Suitable barriers are in place for all the calling workitems to see all the data.
 * - @a sumsSize is a power of 2.
 * - @a threads is a power of 2 which is at least @a sumsSize.
 * - @a lid takes on the values 0, 1, ..., @a threads - 1 across the calling workitems.
 * - The sums do not overflow.
 * @post
 * - All workitems that participated will have visibility of all the results.
 */
inline void upsweep2(__local const WARP_VOLATILE ushort * restrict data, __local WARP_VOLATILE uchar * restrict sums,
                     uint sumsSize, uint lid, uint threads)
{
    if (sumsSize < threads)
        lid &= sumsSize - 1;

    uint in = data[lid];
    uint out = sum2(in);
    sums[lid] = out;

    fastsync(threads); // TODO: could be a smaller number?
}

/**
 * Compute exclusive scan from scanned reduction. The input contains
 * @a fullSize @c uchar values, and @a sumsSize "sums". Each section of
 * @a fullSize / @a sumsSize input values is replaced by its exclusive scan,
 * plus (per-element) the corresponding sum.
 *
 * The function must be called by @a sumsSize workitems, which will
 * cooperatively compute the results.
 *
 * @note It is not required that the data are visible to the calling workitems,
 * and on return the outputs are not guaranteed to be visible to the calling
 * threads.
 *
 * @param[in,out]    data         A <code>uint *</code> alias to the input sequence, replaced by the outputs.
 * @param[in]        sums         The sums to add back to the input sequence.
 * @param            fullSize     The size of the input sequence.
 * @param            sumsSize     The number of sums.
 * @param            lid          ID of the calling workitem.
 *
 * @pre
 * - @a sumsSize is a power of 2.
 * - @a lid takes on the values 0, 1, ..., @a sumsSize - 1 across the calling workitems.
 * - The results do not overflow.
 */
inline void downsweepMulti(
    __local WARP_VOLATILE uint * restrict data,
    __local const WARP_VOLATILE uchar * sums,
    uint fullSize, uint sumsSize, uint lid)
{
    fastsync(sumsSize);
    uint rounds = fullSize / sumsSize / 4;
    uint sum = sums[lid];
    for (int i = 0; i < rounds; i++)
    {
        uint old = data[lid * rounds + i];
        uint m = (old + sum) * 0x01010101;
        data[lid * rounds + i] = m - old;
        sum = m >> 24;
    }
}

/**
 * Compute exclusive scan from scanned factor-4 reduction. The input contains
 * 4 * @a sumsSize @c uchar values, and @a sumsSize "sums". Each section of 4 input
 * values is replaced by its exclusive scan, plus (per-element) the
 * corresponding sum.
 *
 * The function must be called by some number of workitems, which will cooperatively
 * compute the results.
 *
 * @note It is not required that the data are visible to the calling workitems,
 * and on return the outputs are not guaranteed to be visible to the calling
 * threads.
 *
 * @param[in,out]    data         A <code>uint *</code> alias to the input sequence, replaced by the outputs.
 * @param[in]        sums         The sums to add back to the input sequence.
 * @param            sumsSize     The number of sums.
 * @param            lid          ID of the calling workitem.
 * @param            threads      The number of calling workitems.
 * @param            forceZero    If true, the @a sums are ignored and treated as zero.
 *
 * @pre
 * - @a sumsSize is a power of 2.
 * - @a threads is a power of 2 which is at least @a sumsSize.
 * - @a lid takes on the values 0, 1, ..., @a threads - 1 across the calling workitems.
 * - The results do not overflow.
 */
inline void downsweep4(__local WARP_VOLATILE uint * restrict data, __local const WARP_VOLATILE uchar * restrict sums,
                       uint sumsSize, uint lid, uint threads, bool forceZero)
{
    fastsync(threads); // TODO: could be smaller?

    if (sumsSize < threads)
    {
        if (threads <= WARP_SIZE_MEM)
            lid &= sumsSize - 1;
        else if (lid >= sumsSize)
            return;
    }

    uint old = data[lid];
    uint out;
    if (!forceZero)
    {
        uint s = sums[lid];
        out = (old + s) * 0x01010100 + s;
    }
    else
        out = old * 0x01010100;
    data[lid] = out;
}

/**
 * Compute exclusive scan from scanned factor-2 reduction. The input contains
 * 4 * @a sumsSize @c uchar values, and @a sumsSize "sums". Each section of 2 input
 * values is replaced by its exclusive scan, plus (per-element) the
 * corresponding sum.
 *
 * The function must be called by some number of workitems, which will cooperatively
 * compute the results.
 *
 * @note It is not required that the data are visible to the calling workitems,
 * and on return the outputs are not guaranteed to be visible to the calling
 * workitems.
 *
 * @param[in,out]    data         A <code>ushort *</code> alias to the input sequence, replaced by the outputs.
 * @param[in]        sums         The sums to add back to the input sequence.
 * @param            sumsSize     The number of sums.
 * @param            lid          ID of the calling workitem.
 * @param            threads      The number of calling workitems.
 * @param            forceZero    If true, the @a sums are ignored and treated as zero.
 *
 * @pre
 * - @a sumsSize is a power of 2.
 * - @a threads is a power of 2, which is at least @a sumsSize.
 * - @a lid takes on the values 0, 1, ..., @a threads - 1 across the calling workitems.
 * - The results do not overflow.
 */
inline void downsweep2(__local WARP_VOLATILE ushort *restrict data, __local const WARP_VOLATILE uchar * restrict sums,
                       uint sumsSize, uint lid, uint threads, bool forceZero)
{
    fastsync(threads); // TODO: could be smaller?

    if (sumsSize < threads)
    {
        if (threads <= WARP_SIZE_MEM)
            lid &= sumsSize - 1;
        else if (lid >= sumsSize)
            return;
    }

    uint old = data[lid];
    uint out;
    if (!forceZero)
    {
        uint s = sums[lid];
        out = (old + s) * 0x0100 + s;
    }
    else
    {
        out = old * 0x0100;
    }
    data[lid] = out;
}

/**
 * Local data for a single slice of the scatter kernel.
 */
typedef struct
{
    union
    {
        /**
         * Histograms (later scanned) of SCATTER_WORK_SCALE keys and their
         * reductions, using the offsets required by @ref upsweep and
         * @ref downsweep. The bottom-level data are arranged
         * digit-major, workitem-minor.
         *
         * The unions allow for efficient loading of 2 or 4 values at once.
         */
        struct
        {
            union
            {
                uchar c[SCATTER_SLICE * RADIX];
                ushort s[SCATTER_SLICE * RADIX / 2];
                uint i[SCATTER_SLICE * RADIX / 4];
            } level1;

            union
            {
                uchar c[SCATTER_SLICE * 2];
                ushort s[SCATTER_SLICE];
                uint i[SCATTER_SLICE / 2];
            } level2;
        } hist;
#ifdef VALUE_T
        /**
         * Values that are being sorted. This is aliased with
         * the histogram storage purely to reduce local memory storage.
         */
        VALUE_T values[SCATTER_TILE];
#endif
    };

    /// Difference between global and local memory offsets for each digit
    uint bias[RADIX];
    /// The sort digit extracted from the keys
    uchar digits[SCATTER_TILE];
    /**
     * The permutation that needs to be applied. <code>shuf[i]</code> is the
     * original (local) position of the element that must move to (local)
     * position <code>i</code>.
     */
    uchar shuf[SCATTER_TILE];
    /// The sort keys
    KEY_T keys[SCATTER_TILE];
} ScatterData;

/**
 * Scatter a single section of @a SCATTER_SLICE * @a SCATTER_WORK_SCALE input elements.
 *
 * @param[out]     outKeys        Radix-sorted keys.
 * @param[out]     outValues      Values corresponding to @a outKeys.
 * @param[in]      inKeys         Unsorted keys.
 * @param[in]      inValues       Values corresponding to @a inKeys.
 * @param          start          The first input key to process.
 * @param          end            Upper bound on keys to process.
 * @param          firstBit       First bit forming the radix to sort on.
 * @param[in,out]  wg             Local data storage for the slice.
 * @param          lid            ID of this workitem within the slice.
 * @param          offset         The offset into @a outKeys and @a outValues where the
 *                                elements for digit @a lid should be placed
 *                                (undefined if @a lid >= @ref RADIX).
 * @return         The new value for @a offset (incremented by the digit frequency)
 *
 * @pre
 * - @a firstBit < 32.
 * - @a lid takes on the values 0, 1, ..., @ref SCATTER_SLICE once each.
 */
inline uint radixsortScatterTile(
    __global KEY_T *outKeys,
#ifdef VALUE_T
    __global VALUE_T *outValues,
#endif
    __global const KEY_T *inKeys,
#ifdef VALUE_T
    __global const VALUE_T *inValues,
#endif
    uint start,
    uint end,
    uint firstBit,
    __local WARP_VOLATILE ScatterData *wg,
    uint lid,
    uint offset)
{
    // Each workitem processes SCATTER_WORK_SCALE consecutive keys.
    // For each of these, level0 contains the number of previous keys
    // (within that set) that have the same value.
    uint level0[SCATTER_WORK_SCALE];
    // Index into the level1 array corresponding to the ith key processed
    // by the correct workitem.
    uint l1addr[SCATTER_WORK_SCALE];

    /* Load keys and decode digits */
    for (uint i = 0; i < SCATTER_WORK_SCALE; i++)
    {
        const uint kidx = lid + i * SCATTER_SLICE;
        const uint addr = start + kidx;
        const KEY_T key = (addr < end) ? inKeys[addr] : ~(KEY_T) 0;
        const uint digit = (key >> firstBit) & (RADIX - 1);
        wg->keys[kidx] = key;
        wg->digits[kidx] = digit;
    }

    /* Zero out level1 array */
    for (uint i = 0; i < RADIX / 4; i++)
        wg->hist.level1.i[i * SCATTER_SLICE + lid] = 0;

    fastsync(SCATTER_SLICE);

    for (uint i = 0; i < SCATTER_WORK_SCALE; i++)
    {
        const uint kidx = lid * SCATTER_WORK_SCALE + i;
        const uint digit = wg->digits[kidx]; // SCATTER_WORK_SCALE/4-way bank conflict
        l1addr[i] = digit * SCATTER_SLICE + lid;
        level0[i] = wg->hist.level1.c[l1addr[i]];
        wg->hist.level1.c[l1addr[i]] = level0[i] + 1;
    }
    fastsync(SCATTER_SLICE);

    /* Reduce, making sure that we take a stop at RADIX granularity to get digit counts */
    UPSWEEP();

    const uint digitCount = wg->hist.level2.c[RADIX + lid];

    /* Scan */
    DOWNSWEEP();

    fastsync(SCATTER_SLICE);

    /* Compute the relationship between local and global positions.
     * At this point, wg->level1[RADIX + lid] is a scan of the digit counts.
     */
    if (lid < RADIX)
    {
        wg->bias[lid] = offset - wg->hist.level2.c[RADIX + lid]; // conflict-free
        offset += digitCount;
    }

    /* Compute the permutation. level0 gives a workitem-scale scan of
     * SCATTER_WORK_SCALE elements, while level1 contains the higher-level scan.
     */
    for (uint i = 0; i < SCATTER_WORK_SCALE; i++)
    {
        const uint kidx = lid * SCATTER_WORK_SCALE + i;
        uint pos = level0[i] + wg->hist.level1.c[l1addr[i]];
        wg->shuf[pos] = kidx;
    }

    /* values and level1 share memory in a union, so we need this barrier */
    fastsync(SCATTER_SLICE);

#ifdef VALUE_T
    /* Load values */
    for (uint i = 0; i < SCATTER_WORK_SCALE; i++)
    {
        const uint kidx = i * SCATTER_SLICE + lid;
        const uint addr = start + kidx;
        if (addr < end)
            wg->values[kidx] = inValues[addr]; // conflict-free
    }
    fastsync(SCATTER_SLICE);
#endif

    /* Scatter results to global memory */
    for (uint i = 0; i < SCATTER_WORK_SCALE; i++)
    {
        const uint oidx = lid + i * SCATTER_SLICE;
        if ((int) oidx < (int) end - (int) start)
        {
            const uint sh = wg->shuf[oidx];
            const uint digit = wg->digits[sh];
            const uint addr = oidx + wg->bias[digit];
            outKeys[addr] = wg->keys[sh];
#ifdef VALUE_T
            outValues[addr] = wg->values[sh];
#endif
        }
    }

    // The next loop iteration will overwrite the keys, so we need to synchronize here.
    fastsync(SCATTER_SLICE);
    return offset;
}

/**
 * Scatter keys and values into output arrays.
 *
 * @param[out]     outKeys        Radix-sorted keys.
 * @param[out]     outValues      Values corresponding to @a outKeys.
 * @param[in]      inKeys         Unsorted keys.
 * @param[in]      inValues       Values corresponding to @a inKeys.
 * @param[in]      histogram      Scanned histogram computed by @ref radixsortScan.
 * @param          len            Number of keys/values to process per slice.
 * @param          total          Total size of the input and output arrays.
 * @param          firstBit       First bit forming the radix to sort on.
 *
 * @pre
 * - @a histogram contains per-slice offsets indicating where the first
 *   key for each digit should be placed for that slice.
 */
KERNEL(SCATTER_WORK_GROUP_SIZE)
void radixsortScatter(__global KEY_T * restrict outKeys,
                      __global const KEY_T * restrict inKeys,
                      __global const uint *histogram,
                      uint len,
                      uint total,
                      uint firstBit
#ifdef VALUE_T
                      , __global VALUE_T *outValues
                      , __global VALUE_T *inValues
#endif
                     )
{
    __local WARP_VOLATILE ScatterData wd[SCATTER_SLICES];

    const uint local_id = get_local_id(0);
    const uint lid = local_id & (SCATTER_SLICE - 1);
    const uint slice = local_id / SCATTER_SLICE;
    const uint block = get_group_id(0) * SCATTER_SLICES + slice;

    /* Read initial offsets from global memory */
    uint offset = 0;
    if (lid < RADIX)
        offset = histogram[block * RADIX + lid];

    uint start = block * len;
    uint stop = start + len;
    uint end = min(stop, total);
    /* Caution - this loop has to be run the same number of times for each workitem,
     * even if it means that some of them have nothing to do due to end being less
     * than start. Without that, barriers will not operate correctly.
     */
    for (; start < stop; start += SCATTER_TILE)
    {
        offset = radixsortScatterTile(
            outKeys,
#ifdef VALUE_T
            outValues,
#endif
            inKeys,
#ifdef VALUE_T
            inValues,
#endif
            start,
            end,
            firstBit,
            &wd[slice],
            lid,
            offset);
    }

#undef SCATTER_SLICES
}

/********************************************************************************************
 * Pure test code below here. Each function simply loads data into local memory, calls a
 * function, and returns the result back to global memory.
 * TODO: arrange build so that it is only included when building the tests
 ********************************************************************************************/

#ifdef UNIT_TESTS

__kernel void testUpsweep2(__global const uchar *g_data, __global uchar *g_sums, uint dataSize, uint sumsSize)
{
    __local WARP_VOLATILE union
    {
        ushort s[64];
        uchar c[128];
    } data;
    __local WARP_VOLATILE uchar sums[64];

    if (get_local_id(0) == 0)
    {
        for (uint i = 0; i < dataSize; i++)
            data.c[i] = g_data[i];
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    upsweep2(data.s, sums, sumsSize, get_local_id(0), get_local_size(0));

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_local_id(0) == 0)
    {
        for (uint i = 0; i < sumsSize; i++)
            g_sums[i] = sums[i];
    }
}

__kernel void testUpsweep4(__global const uchar *g_data, __global uchar *g_sums, uint dataSize, uint sumsSize)
{
    __local WARP_VOLATILE union
    {
        uint i[64];
        uchar c[256];
    } data;
    __local WARP_VOLATILE uchar sums[64];

    if (get_local_id(0) == 0)
    {
        for (uint i = 0; i < dataSize; i++)
            data.c[i] = g_data[i];
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    upsweep4(data.i, sums, sumsSize, get_local_id(0), get_local_size(0));

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_local_id(0) == 0)
    {
        for (uint i = 0; i < sumsSize; i++)
            g_sums[i] = sums[i];
    }
}

__kernel void testUpsweepMulti(__global const uchar *g_data, __global uchar *g_sums, uint dataSize, uint sumsSize)
{
    __local WARP_VOLATILE union
    {
        uint i[256];
        uchar c[1024];
    } data;
    __local WARP_VOLATILE uchar sums[256];

    if (get_local_id(0) == 0)
    {
        for (uint i = 0; i < dataSize; i++)
            data.c[i] = g_data[i];
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    upsweepMulti(data.i, sums, dataSize, sumsSize, get_local_id(0));

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_local_id(0) == 0)
    {
        for (uint i = 0; i < sumsSize; i++)
            g_sums[i] = sums[i];
    }
}

__kernel void testDownsweep2(__global uchar *g_data, __global const uchar *g_sums, uint dataSize, uint sumsSize, uint forceZero)
{
    __local WARP_VOLATILE union
    {
        ushort s[64];
        uchar c[128];
    } data;
    __local WARP_VOLATILE uchar sums[64];

    if (get_local_id(0) == 0)
    {
        for (uint i = 0; i < dataSize; i++)
            data.c[i] = g_data[i];
        for (uint i = 0; i < sumsSize; i++)
            sums[i] = g_sums[i];
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    downsweep2(data.s, sums, sumsSize, get_local_id(0), get_local_size(0), forceZero);

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_local_id(0) == 0)
    {
        for (uint i = 0; i < dataSize; i++)
        {
            g_data[i] = data.c[i];
        }
    }
}

__kernel void testDownsweep4(__global uchar *g_data, __global const uchar *g_sums, uint dataSize, uint sumsSize, uint forceZero)
{
    __local WARP_VOLATILE union
    {
        uint i[64];
        uchar c[256];
    } data;
    __local WARP_VOLATILE uchar sums[64];

    if (get_local_id(0) == 0)
    {
        for (uint i = 0; i < dataSize; i++)
            data.c[i] = g_data[i];
        for (uint i = 0; i < sumsSize; i++)
            sums[i] = g_sums[i];
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    downsweep4(data.i, sums, sumsSize, get_local_id(0), get_local_size(0), forceZero);

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_local_id(0) == 0)
    {
        for (uint i = 0; i < dataSize; i++)
        {
            g_data[i] = data.c[i];
        }
    }
}

__kernel void testDownsweepMulti(__global uchar *g_data, __global const uchar *g_sums, uint dataSize, uint sumsSize, uint forceZero)
{
    __local WARP_VOLATILE union
    {
        uint i[256];
        uchar c[1024];
    } data;
    __local WARP_VOLATILE uchar sums[256];

    if (get_local_id(0) == 0)
    {
        for (uint i = 0; i < dataSize; i++)
            data.c[i] = g_data[i];
        for (uint i = 0; i < sumsSize; i++)
            sums[i] = g_sums[i];
    }
    barrier(CLK_LOCAL_MEM_FENCE);

    downsweepMulti(data.i, sums, dataSize, sumsSize, get_local_id(0));

    barrier(CLK_LOCAL_MEM_FENCE);

    if (get_local_id(0) == 0)
    {
        for (uint i = 0; i < dataSize; i++)
        {
            g_data[i] = data.c[i];
        }
    }
}

#endif // UNIT_TESTS
