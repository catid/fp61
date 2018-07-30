/*
    Copyright (c) 2018 Christopher A. Taylor.  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of Fp61 nor the names of its contributors may be
      used to endorse or promote products derived from this software without
      specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CAT_FP61_H
#define CAT_FP61_H

#include <stdint.h>


/*
    Integer Arithmetic Modulo Mersenne Prime 2^61-1 in C++

    This software implements arithmetic modulo the Mersenne prime p = 2^61-1.

    It takes advantage of the commonly available fast 64x64->128 multiplier
    to accelerate finite (base) field arithmetic.  So it runs a lot faster
    when built into a 64-bit executable.

    This math code offers full use of lazy reduction techniques,
    via fp61_partial_reduce().

    + Addition of 8 values can be evaluated before reduction.
    + Sums of 4 products can be evaluated with partial reductions.
*/


//------------------------------------------------------------------------------
// Portability Macros

// Compiler-specific force inline keyword
#ifdef _MSC_VER
# define FP61_FORCE_INLINE inline __forceinline
#else
# define FP61_FORCE_INLINE inline __attribute__((always_inline))
#endif


//------------------------------------------------------------------------------
// Portable 64x64->128 Multiply
// CAT_MUL128: r{hi,lo} = x * y

// Returns low part of product, and high part is set in r_hi
FP61_FORCE_INLINE uint64_t Emulate64x64to128(
    uint64_t& r_hi,
    const uint64_t x,
    const uint64_t y)
{
    const uint64_t x0 = (uint32_t)x, x1 = x >> 32;
    const uint64_t y0 = (uint32_t)y, y1 = y >> 32;
    const uint64_t p11 = x1 * y1, p01 = x0 * y1;
    const uint64_t p10 = x1 * y0, p00 = x0 * y0;
    /*
        This is implementing schoolbook multiplication:

                x1 x0
        X       y1 y0
        -------------
                   00  LOW PART
        -------------
                00
             10 10     MIDDLE PART
        +       01
        -------------
             01 
        + 11 11        HIGH PART
        -------------
    */

    // 64-bit product + two 32-bit values
    const uint64_t middle = p10 + (p00 >> 32) + (uint32_t)p01;

    /*
        Proof that 64-bit products can accumulate two more 32-bit values
        without overflowing:

        Max 32-bit value is 2^32 - 1.
        PSum = (2^32-1) * (2^32-1) + (2^32-1) + (2^32-1)
             = 2^64 - 2^32 - 2^32 + 1 + 2^32 - 1 + 2^32 - 1
             = 2^64 - 1
        Therefore it cannot overflow regardless of input.
    */

    // 64-bit product + two 32-bit values
    r_hi = p11 + (middle >> 32) + (p01 >> 32);

    // Add LOW PART and lower half of MIDDLE PART
    return (middle << 32) | (uint32_t)p00;
}

#if defined(_MSC_VER) && defined(_WIN64)
// Visual Studio 64-bit

# include <intrin.h>
# pragma intrinsic(_umul128)
# define CAT_MUL128(r_hi, r_lo, x, y) \
    r_lo = _umul128(x, y, &(r_hi));

#elif defined(__SIZEOF_INT128__)
// Compiler supporting 128-bit values (GCC/Clang)

# define CAT_MUL128(r_hi, r_lo, x, y)                   \
    {                                                   \
        unsigned __int128 w = (unsigned __int128)x * y; \
        r_lo = (uint64_t)w;                             \
        r_hi = (uint64_t)(w >> 64);                     \
    }

#else
// Emulate 64x64->128-bit multiply with 64x64->64 operations

# define CAT_MUL128(r_hi, r_lo, x, y) \
    r_lo = Emulate64x64to128(r_hi, x, y);

#endif // End CAT_MUL128


//------------------------------------------------------------------------------
// Constants

// p = 2^61 - 1
static const uint64_t kFp61Prime = ((uint64_t)1 << 61) - 1;

// Mask where bit #63 is clear and all other bits are set.
static const uint64_t kMask63 = ((uint64_t)1 << 63) - 1;


//------------------------------------------------------------------------------
// API

/**
    fp61_partial_reduce()

    Partially reduce a value (mod p).  This clears bits #63 and #62.

    The result can be passed directly to fp61_add4(), fp61_mul(),
    and fp61_reduce_finalize().
*/
FP61_FORCE_INLINE uint64_t fp61_partial_reduce(uint64_t x)
{
    // Eliminate bits #63 to #61, which may carry back up into bit #61,
    // So we will only definitely reduce #63 and #62.
    return (x & kFp61Prime) + (x >> 61); // 0 <= result <= 2^62 - 1
}

/**
    fp61_reduce_finalize()

    Finalize reduction of a value (mod p) that was partially reduced
    Preconditions: Bits #63 and #62 are clear and x != 0x3ffffffffffffffeULL

    This function fails for x = 0x3ffffffffffffffeULL.
    The partial reduction function does not produce this bit pattern for any
    input, so this exception is allowed because I'm assuming the input comes
    from fp61_partial_reduce().  So, do not mask down to 62 random bits and
    pass to this function because it can fail in this one case.

    Returns a value less than p.
*/
FP61_FORCE_INLINE uint64_t fp61_reduce_finalize(uint64_t x)
{
    // Eliminate #61.
    // The +1 also handles the case where x = p and x = 0x3fffffffffffffffULL.
    // I don't see a way to tweak this to handle 0x3ffffffffffffffeULL...
    return (x + ((x+1) >> 61)) & kFp61Prime; // 0 <= result < p
}

/**
    fp61_add4()

    Sum x + y + z + w (without full reduction modulo p).
    Preconditions: x,y,z,w <2^62

    Probably you will want to just inline this code and follow the pattern,
    since being restricted to adding 4 things at a time is kind of weird.

    The result can be passed directly to fp61_add4(), fp61_mul(), and
    fp61_reduce_finalize().
*/
FP61_FORCE_INLINE uint64_t fp61_add4(uint64_t x, uint64_t y, uint64_t z, uint64_t w)
{
    return fp61_partial_reduce(x + y + z + w);
}

/**
    r = fp61_neg(x)

    r = -x (without reduction modulo p)
    Preconditions: x <= p

    The input needs to be have bits #63 #62 #61 cleared.
    This can be ensured by calling fp61_partial_reduce() and
    fp61_reduce_finalize() first.  Since this is more expensive than addition
    it is best to reorganize operations to avoid needing this reduction.

    Return a value <= p.
*/
FP61_FORCE_INLINE uint64_t fp61_neg(uint64_t x)
{
    return kFp61Prime - x;
}

// For subtraction, use fp61_neg() and fp61_add4().

/**
    r = fp61_mul(x, y)

    r = x * y (with partial reduction modulo p)

    Important Input Restriction:

        The number of bits between x and y must be less than 124 bits.

        Call fp61_partial_reduce() to reduce inputs if needed,
        which makes sure that both inputs are 62 bits or fewer.

        Example: If x <= 2^62-1 (62 bits), then y <= 2^62-1 (62 bits).
        This means that up to 2 values can be accumulated in x and 2 in y.

        But it is also possible to balance the input in other ways.

        Example: If x <= 2^61-1 (61 bits), then y <= 2^63-1 (63 bits).
        This means that up to 4 values can be accumulated in y.

    Result:

        The result is stored in bits #61 to #0 (62 bits of the word).
        Call fp61_final_reduce() to reduce the result to 61 bits.
*/
FP61_FORCE_INLINE uint64_t fp61_mul(uint64_t x, uint64_t y)
{
    uint64_t p_lo, p_hi;
    CAT_MUL128(p_hi, p_lo, x, y);

    /*
        Largest x,y = p - 1 = 2^61 - 2 = L.

        L*L = (2^61-2) * (2^61-2)
            = 2^(61+61) - 4*2^61 + 4
            = 2^122 - 2^63 + 4
        That is the high 6 bits are zero.

        We represent the product as two 64-bit words, or 128 bits.

        Say the low bit bit #64 is set in the high word.
        To eliminate this bit we need to subtract (2^61 - 1) * 2^3.
        This means we need to add a bit at #3.
        Similarly for bit #65 we need to add a bit at #4.

        High bits #127 to #125 affect high bits #66 to #64.
        High bits #124 to #64 affect low bits #63 to #3.
        Low bits #63 to #61 affect low bits #2 to #0.

        If we eliminate from high bits to low bits, then we could carry back
        up into the high bits again.  So we should instead eliminate bits #61
        through #63 first to prevent carries into the high word.
    */

    // Eliminate bits #63 to #61, which may carry back up into bit #61,
    // So we will only definitely reduce #63 and #62.
    uint64_t r = (p_lo & kFp61Prime) + (p_lo >> 61);

    // Eliminate bits #123 to #64 (60 bits).
    // This stops short of #124 that would affect bit #63 because it
    // prevents the addition from overflowing the 64-bit word.
    r += ((p_hi << 3) & kMask63);

    // This last reduction step is not strictly necessary, but is almost always
    // a good idea when used to implement some algorithm, so I include it.
    // Partially reduce the result to clear the high 2 bits.
    return fp61_partial_reduce(r);
}

/**
    r = fp61_inv(x)

    r = x^-1 (mod p)
    The input value x can be any 64-bit value.

    This operation is kind of heavy so it should be avoided where possible.

    This operation is not constant-time.
    A constant-time version can be implemented using Euler's totient method and
    a straight line similar to https://github.com/catid/snowshoe/blob/master/src/fp.inc#L545

    Returns the multiplicative inverse of x modulo p.
    0 < result < p

    If the inverse does not exist, it returns 0.
*/
uint64_t fp61_inv(uint64_t x);


#endif // CAT_FP61_H
