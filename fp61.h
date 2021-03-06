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

/** \mainpage
    Fp61 : Finite field arithmetic modulo Mersenne prime p = 2^61-1 in C++

    The Fp61 software takes advantage of the commonly available fast
    64x64->128 multiplier to accelerate finite (base) field arithmetic.
    So it runs a lot faster when built into a 64-bit executable.

    This math code offers use of lazy reduction techniques for speed,
    via fp61::PartialReduce().

    + Addition of 8 values can be evaluated before reduction.
    + Sums of 4 products can be evaluated with partial reductions.
*/

// Define this to avoid any unaligned memory accesses while reading data.
// This is useful as a quick-fix for mobile applications.
// A preferred solution is to ensure that the data provided is aligned.
// Another reason to do this is if the platform is big-endian.
//#define FP61_SAFE_MEMORY_ACCESSES


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
    // Form temporary 32-bit words
    const uint32_t x0 = static_cast<uint32_t>(x);
    const uint32_t x1 = static_cast<uint32_t>(x >> 32);
    const uint32_t y0 = static_cast<uint32_t>(y);
    const uint32_t y1 = static_cast<uint32_t>(y >> 32);

    // Calculate 32x32->64 bit products
    const uint64_t p11 = static_cast<uint64_t>(x1) * y1;
    const uint64_t p01 = static_cast<uint64_t>(x0) * y1;
    const uint64_t p10 = static_cast<uint64_t>(x1) * y0;
    const uint64_t p00 = static_cast<uint64_t>(x0) * y0;

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
    const uint64_t middle = p10
        + static_cast<uint32_t>(p00 >> 32)
        + static_cast<uint32_t>(p01);

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
    r_hi = p11
        + static_cast<uint32_t>(middle >> 32)
        + static_cast<uint32_t>(p01 >> 32);

    // Add LOW PART and lower half of MIDDLE PART
    return (middle << 32) | static_cast<uint32_t>(p00);
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


namespace fp61 {


//------------------------------------------------------------------------------
// Constants

// p = 2^61 - 1
static const uint64_t kPrime = ((uint64_t)1 << 61) - 1;

// Mask where bit #63 is clear and all other bits are set.
static const uint64_t kMask63 = ((uint64_t)1 << 63) - 1;


//------------------------------------------------------------------------------
// API

/**
    r = fp61::PartialReduce(x)

    Partially reduce x (mod p).  This clears bits #63 and #62.

    The result can be passed directly to fp61::Add4(), fp61::Multiply(),
    and fp61::Finalize().
*/
FP61_FORCE_INLINE uint64_t PartialReduce(uint64_t x)
{
    // Eliminate bits #63 to #61, which may carry back up into bit #61,
    // So we will only definitely reduce #63 and #62.
    return (x & kPrime) + (x >> 61); // 0 <= result <= 2^62 - 1
}

/**
    r = fp61::Finalize(x)

    Finalize reduction of x (mod p) from PartialReduce()
    Preconditions: Bits #63 and #62 are clear and x != 0x3ffffffffffffffeULL

    This function fails for x = 0x3ffffffffffffffeULL.
    The partial reduction function does not produce this bit pattern for any
    input, so this exception is allowed because I'm assuming the input comes
    from fp61::PartialReduce().  So, do not mask down to 62 random bits and
    pass to this function because it can fail in this one case.

    Returns a value in Fp (less than p).
*/
FP61_FORCE_INLINE uint64_t Finalize(uint64_t x)
{
    // Eliminate #61.
    // The +1 also handles the case where x = p and x = 0x3fffffffffffffffULL.
    // I don't see a way to tweak this to handle 0x3ffffffffffffffeULL...
    return (x + ((x+1) >> 61)) & kPrime; // 0 <= result < p
}

/**
    r = fp61::Add4(x, y, z, w)

    Sum x + y + z + w (without full reduction modulo p).
    Preconditions: x,y,z,w <2^62

    Probably you will want to just inline this code and follow the pattern,
    since being restricted to adding 4 things at a time is kind of weird.

    The result can be passed directly to fp61::Add4(), fp61::Multiply(), and
    fp61::Finalize().
*/
FP61_FORCE_INLINE uint64_t Add4(uint64_t x, uint64_t y, uint64_t z, uint64_t w)
{
    return PartialReduce(x + y + z + w);
}

/**
    r = fp61::Negate(x)

    r = -x (without reduction modulo p)
    Preconditions: x <= p

    The input needs to be have bits #63 #62 #61 cleared.
    This can be ensured by calling fp61::PartialReduce() and
    fp61::Finalize() first.  Since this is more expensive than addition
    it is best to reorganize operations to avoid needing this reduction.

    Return a value <= p.
*/
FP61_FORCE_INLINE uint64_t Negate(uint64_t x)
{
    return kPrime - x;
}

// For subtraction, use fp61::Negate() and add: x + (-y).

/**
    r = fp61::Multiply(x, y)

    r = x * y (with partial reduction modulo p)

    Important Input Restriction:

        The number of bits between x and y must be less than 124 bits.

        Call fp61::PartialReduce() to reduce inputs if needed,
        which makes sure that both inputs are 62 bits or fewer.

        Example: If x <= 2^62-1 (62 bits), then y <= 2^62-1 (62 bits).
        This means that up to 2 values can be accumulated in x and 2 in y.

        But it is also possible to balance the input in other ways.

        Example: If x <= 2^61-1 (61 bits), then y <= 2^63-1 (63 bits).
        This means that up to 4 values can be accumulated in y.

    Result:

        The result is stored in bits #61 to #0 (62 bits of the word).
        Call fp61::Finalize() to reduce the result to 61 bits.
*/
FP61_FORCE_INLINE uint64_t Multiply(uint64_t x, uint64_t y)
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
    uint64_t r = (p_lo & kPrime) + (p_lo >> 61);

    // Eliminate bits #123 to #64 (60 bits).
    // This stops short of #124 that would affect bit #63 because it
    // prevents the addition from overflowing the 64-bit word.
    r += ((p_hi << 3) & kMask63);

    // This last reduction step is not strictly necessary, but is almost always
    // a good idea when used to implement some algorithm, so I include it.
    // Partially reduce the result to clear the high 2 bits.
    return PartialReduce(r);
}

/**
    r = fp61::Inverse(x)

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
uint64_t Inverse(uint64_t x);


//------------------------------------------------------------------------------
// Memory Reading

/// Read 8 bytes in little-endian byte order
FP61_FORCE_INLINE uint64_t ReadU64_LE(const uint8_t* data)
{
#ifdef FP61_SAFE_MEMORY_ACCESSES
    return ((uint64_t)data[7] << 56) | ((uint64_t)data[6] << 48) | ((uint64_t)data[5] << 40) |
        ((uint64_t)data[4] << 32) | ((uint64_t)data[3] << 24) | ((uint64_t)data[2] << 16) |
        ((uint64_t)data[1] << 8) | data[0];
#else
    const uint64_t* wordPtr = reinterpret_cast<const uint64_t*>(data);
    return *wordPtr;
#endif
}

/// Read 4 bytes in little-endian byte order
FP61_FORCE_INLINE uint32_t ReadU32_LE(const uint8_t* data)
{
#ifdef FP61_SAFE_MEMORY_ACCESSES
    return ((uint32_t)data[3] << 24) | ((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | data[0];
#else
    const uint32_t* wordPtr = reinterpret_cast<const uint32_t*>(data);
    return *wordPtr;
#endif
}

/// Read between 0..8 bytes in little-endian byte order
/// Returns 0 for any other value for `bytes`
uint64_t ReadBytes_LE(const uint8_t* data, unsigned bytes);

enum class ReadResult
{
    Success, ///< Read returned with a word of data
    Empty    ///< No data remaining to read
};

/**
    Fitting Bytes Into Words

    When converting byte data to words, a value of 2^61-1 is problematic
    because it does not fit in the field Fp that ranges from 0..(2^61-2).

    One way to fit these values into the field would be to emit 1ff..ffe
    for both 1ff..ffe and 1ff..fff, and then inject a new bit after it to
    allow the ByteWriter code to reverse the transformation.  The problem
    with this is that the lower bit is modified, which is the same one
    that signals how the prior word is modified.

    So a better way to fix 1ff..fff is to make it ambiguous with 0ff..fff,
    where the high bit of the word is flipped.  Now when 0ff..fff is seen
    by the ByteWriter, it knows to check the next word's low bit and
    optionally reverse it back to 1ff..fff.

    As an aside, we want to design the ByteReader to be as fast as possible
    because it is used by the erasure code encoder - The decoder must only
    reverse this transformation for any lost data, so it can be slower.

    It may be a good idea to XOR input data by a random sequence to randomize
    the odds of using extra bits, depending on the application.
*/
static const uint64_t kAmbiguityMask = ((uint64_t)1 << 60) - 1; // 0x0ff...fff

/// Returns true if the U64 word provided needs an extra bit to represent it
FP61_FORCE_INLINE bool IsU64Ambiguous(uint64_t u64_word)
{
    return (u64_word & kAmbiguityMask) == kAmbiguityMask;
}

/// Returns true if this Fp word could have originally been 0ff..ff or 1ff..ff
FP61_FORCE_INLINE bool IsFpAmbiguous(uint64_t fp_word)
{
    return fp_word == kAmbiguityMask;
}

/**
    ByteReader

    Reads 8 bytes at a time from the input data and outputs 61-bit Fp words.
    Pads the final < 8 bytes with zeros.

    See the comments on Fitting Bytes Into Words for how this works.

    Call ByteReader::MaxWords() to calculate the maximum number of words that
    can be generated for worst-case input of all FFF...FFs.

    Define FP61_SAFE_MEMORY_ACCESSES if the platform does not support unaligned
    reads and the input data is unaligned, or the platform is big-endian.

    Call BeginRead() to begin reading.

    Call ReadNext() repeatedly to read all words from the data.
    It will return ReadResult::Empty when all bits are empty.
*/
struct ByteReader
{
    const uint8_t* Data;
    unsigned Bytes;
    uint64_t Workspace;
    int Available;


    /// Calculates and returns the maximum number of Fp field words that may be
    /// produced by the ByteReader.
    static FP61_FORCE_INLINE unsigned MaxWords(unsigned bytes)
    {
        unsigned bits = bytes * 8;

        // Round up to the nearest word.
        // All words may be expanded by one bit, hence the (bits/61) factor.
        return (bits + (bits / 61) + 60) / 61;
    }

    /// Begin reading data
    FP61_FORCE_INLINE void BeginRead(const uint8_t* data, unsigned bytes)
    {
        Data = data;
        Bytes = bytes;
        Workspace = 0;
        Available = 0;
    }

    /// Returns ReadResult::Empty when no more data is available.
    /// Otherwise fpOut will be a value between 0 and p-1.
    ReadResult Read(uint64_t& fpOut);
};

/**
    WordReader

    Reads a series of 61-bit finalized Fp field elements from a byte array.

    This differs from ByteReader in two ways:
    (1) It does not have to handle the special case of all ffffs.
    (2) It terminates deterministically at WordCount() words rather than
    based on the contents of the data.

    Call WordCount() to calculate the number of words to expect to read from
    a given number of bytes.

    Call BeginRead() to start reading.
    Call Read() to retrieve each consecutive word.
*/
struct WordReader
{
    const uint8_t* Data;
    unsigned Bytes;
    uint64_t Workspace;
    unsigned Available;


    /// Calculate the number of words that can be read from a number of bytes
    static FP61_FORCE_INLINE unsigned WordCount(unsigned bytes)
    {
        // Note that only whole (not partial) words can be read, so this rounds down
        return (bytes * 8) / 61;
    }

    /// Begin writing to the given memory location
    FP61_FORCE_INLINE void BeginRead(const uint8_t* data, unsigned bytes)
    {
        Data = data;
        Bytes = bytes;
        Workspace = 0;
        Available = 0;
    }

    /// Read the next word.
    /// It is up to the application to know when to stop reading,
    /// based on the WordCount() count of words to read.
    uint64_t Read();
};


//------------------------------------------------------------------------------
// Memory Writing

/// Write 4 bytes in little-endian byte order
FP61_FORCE_INLINE void WriteU32_LE(uint8_t* data, uint32_t value)
{
#ifdef FP61_SAFE_MEMORY_ACCESSES
    data[3] = (uint8_t)(value >> 24);
    data[2] = (uint8_t)(value >> 16);
    data[1] = (uint8_t)(value >> 8);
    data[0] = (uint8_t)value;
#else
    uint32_t* wordPtr = reinterpret_cast<uint32_t*>(data);
    *wordPtr = value;
#endif
}

/// Write 8 bytes in little-endian byte order
FP61_FORCE_INLINE void WriteU64_LE(uint8_t* data, uint64_t value)
{
#ifdef FP61_SAFE_MEMORY_ACCESSES
    data[7] = (uint8_t)(value >> 56);
    data[6] = (uint8_t)(value >> 48);
    data[5] = (uint8_t)(value >> 40);
    data[4] = (uint8_t)(value >> 32);
    data[3] = (uint8_t)(value >> 24);
    data[2] = (uint8_t)(value >> 16);
    data[1] = (uint8_t)(value >> 8);
    data[0] = (uint8_t)value;
#else
    uint64_t* wordPtr = reinterpret_cast<uint64_t*>(data);
    *wordPtr = value;
#endif
}

/// Write between 0..8 bytes in little-endian byte order
void WriteBytes_LE(uint8_t* data, unsigned bytes, uint64_t value);

/**
    WordWriter

    Writes a series of 61-bit finalized Fp field elements to a byte array.
    The resulting data can be read by WordReader.

    Call BytesNeeded() to calculate the number of bytes needed to store the
    given number of Fp words.

    Call BeginWrite() to start writing.
    Call Write() to write the next word.

    Call Flush() to write the last few bytes.
    Flush() returns the number of overall written bytes.
*/
struct WordWriter
{
    uint8_t* Data;
    uint8_t* DataWritePtr;
    uint64_t Workspace;
    unsigned Available;


    /// Calculate the number of bytes that will be written
    /// for the given number of Fp words.
    static FP61_FORCE_INLINE unsigned BytesNeeded(unsigned words)
    {
        // 61 bits per word
        const unsigned bits = words * 61;

        // Round up to the next byte
        return (bits + 7) / 8;
    }

    /// Begin writing to the given memory location.
    /// It is up to the application to provide enough space in the buffer by
    /// using BytesNeeded() to calculate the buffer size.
    FP61_FORCE_INLINE void BeginWrite(uint8_t* data)
    {
        Data = data;
        DataWritePtr = data;
        Workspace = 0;
        Available = 0;
    }

    /// Write the next word
    FP61_FORCE_INLINE void Write(uint64_t word)
    {
        unsigned available = Available;
        uint64_t workspace = Workspace;

        // Include any bits that fit
        workspace |= word << available;
        available += 61;

        // If there is a full word now:
        if (available >= 64)
        {
            // Write the word
            WriteU64_LE(DataWritePtr, workspace);
            DataWritePtr += 8;
            available -= 64;

            // Keep remaining bits
            workspace = word >> (61 - available);
        }

        Workspace = workspace;
        Available = available;
    }

    /// Flush the output, writing fractions of a word if needed.
    /// This must be called or the output may be truncated.
    /// Returns the number of bytes written overall.
    FP61_FORCE_INLINE unsigned Flush()
    {
        const unsigned finalBytes = (Available + 7) / 8;

        // Write the number of available bytes
        WriteBytes_LE(DataWritePtr, finalBytes, Workspace);

        // Calculate number of bytes written overall
        const uintptr_t writtenBytes = static_cast<uintptr_t>(DataWritePtr - Data) + finalBytes;

        return static_cast<unsigned>(writtenBytes);
    }
};

/**
    ByteWriter

    Writes a series of 61-bit finalized Fp field elements to a byte array,
    reversing the encoding of ByteReader.  This is different from WordWriter
    because it can also write 61-bit values that are all ones (outside of Fp).

    See the comments on Fitting Bytes Into Words for how this works.

    Call MaxBytesNeeded() to calculate the maximum number of bytes needed
    to store the given number of Fp words.

    Call BeginWrite() to start writing.
    Call Write() to write the next word.

    Call Flush() to write the last few bytes.
    Flush() returns the number of overall written bytes.
*/
struct ByteWriter
{
    WordWriter Writer;
    bool Packed;

    /// Calculate the maximum number of bytes that will be written for the
    /// given number of Fp words.  May be up to 1.6% larger than necessary.
    static FP61_FORCE_INLINE unsigned MaxBytesNeeded(unsigned words)
    {
        return WordWriter::BytesNeeded(words);
    }

    /// Begin writing to the given memory location.
    /// It is up to the application to provide enough space in the buffer by
    /// using MaxBytesNeeded() to calculate the buffer size.
    FP61_FORCE_INLINE void BeginWrite(uint8_t* data)
    {
        Writer.BeginWrite(data);
        Packed = false;
    }

    /// Write the next word
    FP61_FORCE_INLINE void Write(uint64_t word)
    {
        const unsigned word_bits = (word == kAmbiguityMask) ? 60 : 61;

        unsigned available = Writer.Available;
        uint64_t workspace = Writer.Workspace;

        // Include any bits that fit
        workspace |= word << available;
        available += word_bits;

        // If there is a full word now:
        if (available >= 64)
        {
            // Write the word
            WriteU64_LE(Writer.DataWritePtr, workspace);
            Writer.DataWritePtr += 8;
            available -= 64;

            // Keep remaining bits
            workspace = word >> (word_bits - available);
        }

        Writer.Workspace = workspace;
        Writer.Available = available;
    }

    /// Flush the output, writing fractions of a word if needed.
    /// This must be called or the output may be truncated.
    /// Returns the number of bytes written overall.
    FP61_FORCE_INLINE unsigned Flush()
    {
        return Writer.Flush();
    }
};


//------------------------------------------------------------------------------
// Random Numbers

#define CAT_ROL64(x, bits) ( ((uint64_t)(x) << (bits)) | ((uint64_t)(x) >> (64 - (bits))) )

/**
    Random

    Xoshiro256+ based pseudo-random number generator (PRNG) that can generate
    random numbers between 1..p.  NextNonzeroFp() is mainly intended to be used
    for producing convolutional code coefficients to multiply by the data.

    Call Seed() to provide a 64-bit generator seed.
    Call NextNonzeroFp() to produce a random 61-bit number from 1..p
    Call NextFp() to produce a random 61-bit number from 0..p
    Call Next() to produce a random 64-bit number.
*/
struct Random
{
    uint64_t State[4];


    /// Seed the generator
    void Seed(uint64_t x);

    /// Get the next 64-bit random number.
    /// The low 3 bits are slightly weak according to the authors.
    // From http://xoshiro.di.unimi.it/xoshiro256plus.c
    // Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)
    FP61_FORCE_INLINE uint64_t Next()
    {
        uint64_t s0 = State[0], s1 = State[1], s2 = State[2], s3 = State[3];

        const uint64_t result = s0 + s3;

        const uint64_t t = s1 << 17;
        s2 ^= s0;
        s3 ^= s1;
        s1 ^= s2;
        s0 ^= s3;
        s2 ^= t;
        s3 = CAT_ROL64(s3, 45);

        State[0] = s0, State[1] = s1, State[2] = s2, State[3] = s3;

        return result;
    }

    static FP61_FORCE_INLINE uint64_t ConvertRandToFp(uint64_t word)
    {
        // Pick high bits as recommended by Xoshiro authors
        word >>= 3;

        // If word + 1 overflows, then subtract 1.
        // This converts fffff to ffffe and slightly biases the PRNG.
        word -= (word + 1) >> 61;

        return word;
    }

    static FP61_FORCE_INLINE uint64_t ConvertRandToNonzeroFp(uint64_t word)
    {
        word = ConvertRandToFp(word);

        // If word - 1 borrows out, then add 1.
        // This converts 0 to 1 and slightly biases the PRNG.
        word += (word - 1) >> 63;

        return word;
    }

    /// Get the next random value between 0..p
    FP61_FORCE_INLINE uint64_t NextFp()
    {
        return ConvertRandToFp(Next());
    }

    /// Get the next random value between 1..p
    FP61_FORCE_INLINE uint64_t NextNonzeroFp()
    {
        return ConvertRandToNonzeroFp(Next());
    }
};

/// Hash a 64-bit value to another 64-bit value
uint64_t HashU64(uint64_t x);

/// Hash a seed into a value from 1..p-1
FP61_FORCE_INLINE uint64_t HashToNonzeroFp(uint64_t word)
{
    // Run a simple mixer based on HashU64()
    word += 0x9e3779b97f4a7c15;
    word = (word ^ (word >> 30)) * 0xbf58476d1ce4e5b9;

    // Take the top 61 bits
    word >>= 3;

    // Eliminate values = p
    word -= (word + 1) >> 61;

    // Eliminate values = 0
    word += (word - 1) >> 63;

    return word;
}


} // namespace fp61


#endif // CAT_FP61_H
