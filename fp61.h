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
    via fp61_partial_reduce() and fp61_reduce().
    + Addition of 8 values can be evaluated before reduction.
    + Sums of 4 products can be evaluated with partial reductions.
*/


//------------------------------------------------------------------------------
// Portable 64x64->128 Multiply
// CAT_MUL128: r{hi,lo} = x * y

#if defined(_MSC_VER) && defined(_WIN64)
# include <intrin.h>
# pragma intrinsic(_umul128)
# define CAT_MUL128(r_hi, r_lo, x, y) \
    r_lo = _umul128(x, y, &(r_hi));
#elif defined(__SIZEOF_INT128__) // GCC, Clang, etc:
    typedef __uint128_t u128;
# define CAT_MUL128(r_hi, r_lo, x, y) \
    {                                 \
        u128 w = (u128)x * y;         \
        r_lo = (uint64_t)w;           \
        r_hi = (uint64_t)(w >> 64);   \
    }
#else // Emulate 64-bit multiply:
# define CAT_MUL128(r_hi, r_lo, x, y)                    \
    {                                                    \
        r_lo = (uint64_t)x * y;                          \
        uint64_t ac = (uint64_t)(x >> 32) * (y >> 32);   \
        uint64_t ad = (uint64_t)(x >> 32) * (uint32_t)y; \
        uint64_t bc = (uint64_t)(y >> 32) * (uint32_t)x; \
        r_hi = ac + ((ad + bc) >> 32);                   \
    }
#endif


//------------------------------------------------------------------------------
// Constants

// p = 2^61 - 1
static const uint64_t kFp61Prime = ((uint64_t)1 << 61) - 1;

// Mask where bit #63 is clear and all other bits are set.
static const uint64_t kMask63 = ((uint64_t)1 << 63) - 1;


//------------------------------------------------------------------------------
// API

// x = -x (without reduction modulo p)
// Preconditions: x <= p
// The result will be <= p
inline uint64_t fp61_neg(uint64_t x)
{
    return kFp61Prime - x;
}

// Partially reduce a value (mod p)
// This clears bits #63 and #62.
// The result can be passed directly to fp61_add4() or fp61_mul().
inline uint64_t fp61_partial_reduce(uint64_t x)
{
    // Eliminate bits #63 to #61, which may carry back up into bit #61,
    // So we will only definitely reduce #63 and #62.
    return (x & kFp61Prime) + (x >> 61); // 0 <= result <= 2^62 - 1
}

// Finalize reduction of a value (mod p) that was partially reduced
// Preconditions: Bits #63 and #62 are clear.
// The result is less than p.
inline uint64_t fp61_reduce_finalize(uint64_t x)
{
    // Eliminate #61.  The +1 also handles the case where x = p.
    return (x + ((x + 1) >> 61)) & kFp61Prime; // 0 <= result < p
}

// x + y (without full reduction modulo p).
// Preconditions: x,y,z,w <2^62
// The result can be passed directly to fp61_add4() or fp61_mul().
inline uint64_t fp61_add4(uint64_t x, uint64_t y, uint64_t z, uint64_t w)
{
    return fp61_partial_reduce(x + y + z + w);
}

// For subtraction, use fp61_neg() and fp61_add4().

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

/*
    x * y (without reduction modulo p)

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
inline uint64_t fp61_mul(uint64_t x, uint64_t y)
{
    uint64_t p_lo, p_hi;
    CAT_MUL128(p_hi, p_lo, x, y);

    // Eliminate bits #63 to #61, which may carry back up into bit #61,
    // So we will only definitely reduce #63 and #62.
    uint64_t r = (p_lo & kFp61Prime) + (p_lo >> 61);

    // Eliminate bits #123 to #64 (60 bits).
    // This stops short of #124 that would affect bit #63 because it
    // prevents the addition from overflowing the 64-bit word.
    r += ((p_hi << 3) & kMask63);

    // Partially reduce the result to clear the high 2 bits.
    return fp61_partial_reduce(r);
}

// x^-1 (mod p)
// Precondition: 0 < x < p
// Call fp61_reduce() if needed to ensure the precondition.
// This operation is not constant-time.
uint64_t fp61_inv(uint64_t x);


#endif // CAT_FP61_H
