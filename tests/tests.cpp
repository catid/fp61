#include "../fp61.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
using namespace std;


//------------------------------------------------------------------------------
// Portability macros

// Compiler-specific debug break
#if defined(_DEBUG) || defined(DEBUG)
    #define FP61_DEBUG
    #ifdef _WIN32
        #define FP61_DEBUG_BREAK() __debugbreak()
    #else
        #define FP61_DEBUG_BREAK() __builtin_trap()
    #endif
    #define FP61_DEBUG_ASSERT(cond) { if (!(cond)) { FP61_DEBUG_BREAK(); } }
#else
    #define FP61_DEBUG_BREAK() do {} while (false);
    #define FP61_DEBUG_ASSERT(cond) do {} while (false);
#endif


//------------------------------------------------------------------------------
// Constants

#define FP61_RET_FAIL -1
#define FP61_RET_SUCCESS 0

static const uint64_t MASK61 = ((uint64_t)1 << 61) - 1;
static const uint64_t MASK62 = ((uint64_t)1 << 62) - 1;
static const uint64_t MASK63 = ((uint64_t)1 << 63) - 1;
static const uint64_t MASK64 = ~(uint64_t)0;
static const uint64_t MASK64_NO62 = MASK64 ^ ((uint64_t)1 << 62);
static const uint64_t MASK64_NO61 = MASK64 ^ ((uint64_t)1 << 61);
static const uint64_t MASK64_NO60 = MASK64 ^ ((uint64_t)1 << 60);
static const uint64_t MASK63_NO61 = MASK63 ^ ((uint64_t)1 << 61);
static const uint64_t MASK63_NO60 = MASK63 ^ ((uint64_t)1 << 60);
static const uint64_t MASK62_NO60 = MASK62 ^ ((uint64_t)1 << 60);

#if defined(FP61_DEBUG)
static const unsigned kRandomTestLoops = 100000;
static const unsigned kMaxDataLength = 4000;
#else
static const unsigned kRandomTestLoops = 10000000;
static const unsigned kMaxDataLength = 30000;
#endif


//------------------------------------------------------------------------------
// Tools

static std::string HexString(uint64_t x)
{
    std::stringstream ss;
    ss << hex << setfill('0') << setw(16) << x;
    return ss.str();
}


//------------------------------------------------------------------------------
// Tests: Negate

static bool test_negate(uint64_t x)
{
    uint64_t n = fp61::Negate(x);
    uint64_t s = (x + n) % fp61::kPrime;
    if (s != 0) {
        cout << "Failed for x = " << hex << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }
    return true;
}

static bool TestNegate()
{
    cout << "TestNegate...";

    // Input is allowed to be 0 <= x <= p
    for (uint64_t x = 0; x < 1000; ++x) {
        if (!test_negate(x)) {
            return false;
        }
    }
    for (uint64_t x = fp61::kPrime; x >= fp61::kPrime - 1000; --x) {
        if (!test_negate(x)) {
            return false;
        }
    }

    fp61::Random prng;
    prng.Seed(1);

    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next() & fp61::kPrime;
        if (!test_negate(x)) {
            return false;
        }
    }

    cout << "Passed" << endl;

    return true;
}


//------------------------------------------------------------------------------
// Tests: Add

static bool TestAdd()
{
    cout << "TestAdd...";

    // Preconditions: x,y,z,w <2^62
    const uint64_t largest = ((uint64_t)1 << 62) - 1;
    const uint64_t reduced = largest % fp61::kPrime;

    for (uint64_t x = largest; x >= largest - 1000; --x)
    {
        uint64_t r = fp61::Add4(largest, largest, largest, x);

        uint64_t expected = 0;
        expected = (expected + reduced) % fp61::kPrime;
        expected = (expected + reduced) % fp61::kPrime;
        expected = (expected + reduced) % fp61::kPrime;
        expected = (expected + (x % fp61::kPrime)) % fp61::kPrime;

        if (r % fp61::kPrime != expected) {
            cout << "Failed for x = " << HexString(x) << endl;
            FP61_DEBUG_BREAK();
            return false;
        }
    }

    for (uint64_t x = largest; x >= largest - 1000; --x)
    {
        for (uint64_t y = largest; y >= largest - 1000; --y)
        {
            uint64_t r = fp61::Add4(largest, largest, x, y);

            uint64_t expected = 0;
            expected = (expected + reduced) % fp61::kPrime;
            expected = (expected + reduced) % fp61::kPrime;
            expected = (expected + (y % fp61::kPrime)) % fp61::kPrime;
            expected = (expected + (x % fp61::kPrime)) % fp61::kPrime;

            if (r % fp61::kPrime != expected) {
                cout << "Failed for x=" << HexString(x) << " y=" << HexString(y) << endl;
                FP61_DEBUG_BREAK();
                return false;
            }
        }
    }

    fp61::Random prng;
    prng.Seed(0);

    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        // Select 4 values from 0..2^62-1
        uint64_t x = prng.Next() & MASK62;
        uint64_t y = prng.Next() & MASK62;
        uint64_t w = prng.Next() & MASK62;
        uint64_t z = prng.Next() & MASK62;

        uint64_t r = fp61::Add4(x, y, z, w);

        uint64_t expected = 0;
        expected = (expected + (x % fp61::kPrime)) % fp61::kPrime;
        expected = (expected + (y % fp61::kPrime)) % fp61::kPrime;
        expected = (expected + (z % fp61::kPrime)) % fp61::kPrime;
        expected = (expected + (w % fp61::kPrime)) % fp61::kPrime;

        if (r % fp61::kPrime != expected) {
            cout << "Failed (random) for i = " << i << endl;
            FP61_DEBUG_BREAK();
            return false;
        }
    }

    cout << "Passed" << endl;

    return true;
}


//------------------------------------------------------------------------------
// Tests: Partial Reduction

static bool test_pred(uint64_t x)
{
    uint64_t expected = x % fp61::kPrime;

    uint64_t r = fp61::PartialReduce(x);

    if ((r >> 62) != 0)
    {
        cout << "High bit overflow failed for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    uint64_t actual = fp61::PartialReduce(x) % fp61::kPrime;

    if (actual != expected)
    {
        cout << "Failed for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }
    return true;
}

static bool TestPartialReduction()
{
    cout << "TestPartialReduction...";

    // Input can have any bit set

    for (uint64_t x = 0; x < 1000; ++x) {
        if (!test_pred(x)) {
            return false;
        }
    }
    for (uint64_t x = MASK64; x > MASK64 - 1000; --x) {
        if (!test_pred(x)) {
            return false;
        }
    }
    for (uint64_t x = MASK64_NO62 + 1000; x > MASK64_NO62 - 1000; --x) {
        if (!test_pred(x)) {
            return false;
        }
    }
    for (uint64_t x = MASK64_NO61 + 1000; x > MASK64_NO61 - 1000; --x) {
        if (!test_pred(x)) {
            return false;
        }
    }
    for (uint64_t x = MASK64_NO60 + 1000; x > MASK64_NO60 - 1000; --x) {
        if (!test_pred(x)) {
            return false;
        }
    }
    for (uint64_t x = MASK63; x > MASK63 - 1000; --x) {
        if (!test_pred(x)) {
            return false;
        }
    }
    for (uint64_t x = MASK63_NO61 + 1000; x > MASK63_NO61 - 1000; --x) {
        if (!test_pred(x)) {
            return false;
        }
    }
    for (uint64_t x = MASK63_NO60 + 1000; x > MASK63_NO60 - 1000; --x) {
        if (!test_pred(x)) {
            return false;
        }
    }
    for (uint64_t x = MASK62 + 1000; x > MASK62 - 1000; --x) {
        if (!test_pred(x)) {
            return false;
        }
    }
    for (uint64_t x = MASK62_NO60 + 1000; x > MASK62_NO60 - 1000; --x) {
        if (!test_pred(x)) {
            return false;
        }
    }
    for (uint64_t x = MASK61 + 1000; x > MASK61 - 1000; --x) {
        if (!test_pred(x)) {
            return false;
        }
    }

    fp61::Random prng;
    prng.Seed(2);

    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next();

        if (!test_pred(x)) {
            return false;
        }
    }

    cout << "Passed" << endl;

    return true;
}


//------------------------------------------------------------------------------
// Tests: Finalize Reduction

static bool test_fred(uint64_t x)
{
    // EXCEPTION: This input is known to not work
    if (x == 0x3ffffffffffffffeULL) {
        return true;
    }

    uint64_t actual = fp61::Finalize(x);
    uint64_t expected = x % fp61::kPrime;

    if (actual != expected)
    {
        cout << "Failed for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }
    return true;
}

static bool TestFinalizeReduction()
{
    cout << "TestFinalizeReduction...";

    // Input has #63 and #62 clear, other bits can take on any value

    for (uint64_t x = 0; x < 1000; ++x) {
        if (!test_fred(x)) {
            return false;
        }
    }
    for (uint64_t x = MASK62; x > MASK62 - 1000; --x) {
        if (!test_fred(x)) {
            return false;
        }
    }
    for (uint64_t x = MASK62_NO60 + 1000; x > MASK62_NO60 - 1000; --x) {
        if (!test_fred(x)) {
            return false;
        }
    }
    for (uint64_t x = MASK61 + 1000; x > MASK61 - 1000; --x) {
        if (!test_fred(x)) {
            return false;
        }
    }

    fp61::Random prng;
    prng.Seed(3);

    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next() & MASK62;

        if (!test_fred(x)) {
            return false;
        }
    }

    cout << "Passed" << endl;

    return true;
}


//------------------------------------------------------------------------------
// Tests: Multiply

static bool test_mul(uint64_t x, uint64_t y)
{
    uint64_t p = fp61::Multiply(x, y);

    if ((p >> 62) != 0) {
        cout << "Failed (high bit overflow) for x=" << HexString(x) << ", y=" << HexString(y) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    uint64_t r0, r1;
    CAT_MUL128(r1, r0, x, y);

    //A % B == (((AH % B) * (2^64 % B)) + (AL % B)) % B
    //  == (((AH % B) * ((2^64 - B) % B)) + (AL % B)) % B
    r1 %= fp61::kPrime;
    uint64_t NB = (uint64_t)(-(int64_t)fp61::kPrime);
    uint64_t mod = r1 * (NB % fp61::kPrime);
    mod += r0 % fp61::kPrime;
    mod %= fp61::kPrime;

    if (p % fp61::kPrime != mod) {
        cout << "Failed (reduced result mismatch) for x=" << HexString(x) << ", y=" << HexString(y) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    return true;
}

static bool TestMultiply()
{
    cout << "TestMultiply...";

    // Number of bits between x, y must be 124 or fewer.

    for (uint64_t x = 0; x < 1000; ++x) {
        for (uint64_t y = x; y < 1000; ++y) {
            if (!test_mul(x, y)) {
                return false;
            }
        }
    }
    for (uint64_t x = MASK62; x > MASK62 - 1000; --x) {
        for (uint64_t y = x; y > MASK62 - 1000; --y) {
            if (!test_mul(x, y)) {
                return false;
            }
        }
    }
    for (uint64_t x = MASK62_NO60 + 1000; x > MASK62_NO60 - 1000; --x) {
        for (uint64_t y = x; y > MASK62_NO60 - 1000; --y) {
            if (!test_mul(x, y)) {
                return false;
            }
        }
    }
    for (uint64_t x = MASK61 + 1000; x > MASK61 - 1000; --x) {
        for (uint64_t y = x; y > MASK61 - 1000; --y) {
            if (!test_mul(x, y)) {
                return false;
            }
        }
    }

    fp61::Random prng;
    prng.Seed(4);

    // 62 + 62 = 124 bits
    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next() & MASK62;
        uint64_t y = prng.Next() & MASK62;

        if (!test_mul(x, y)) {
            return false;
        }
    }

    // 61 + 63 = 124 bits
    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next() & MASK61;
        uint64_t y = prng.Next() & MASK63;

        if (!test_mul(x, y)) {
            return false;
        }
    }

    // Commutivity test
    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next() & MASK62;
        uint64_t y = prng.Next() & MASK62;
        uint64_t z = prng.Next() & MASK62;

        uint64_t r = fp61::Finalize(fp61::Multiply(fp61::Multiply(z, y), x));
        uint64_t s = fp61::Finalize(fp61::Multiply(fp61::Multiply(x, z), y));
        uint64_t t = fp61::Finalize(fp61::Multiply(fp61::Multiply(x, y), z));

        if (r != s || s != t) {
            cout << "Failed (does not commute) for i=" << i << endl;
            FP61_DEBUG_BREAK();
            return false;
        }
    }

    // Direct function test
    uint64_t r1, r0;
    r0 = Emulate64x64to128(r1, MASK64, MASK64);

    if (r1 != 0xfffffffffffffffe || r0 != 1) {
        cout << "Failed (Emulate64x64to128 failed)" << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    cout << "Passed" << endl;

    return true;
}


//------------------------------------------------------------------------------
// Tests: Inverse

static bool test_inv(uint64_t x)
{
    uint64_t i = fp61::Inverse(x);

    // If no inverse existed:
    if (i == 0)
    {
        // Then it must have evenly divided
        if (x % fp61::kPrime == 0) {
            return true;
        }

        // Otherwise this should have had a result
        cout << "Failed (no result) for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    // Result must be in Fp
    if (i >= fp61::kPrime)
    {
        cout << "Failed (result too large) for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    // mul requires partially reduced input
    x = fp61::PartialReduce(x);

    uint64_t p = fp61::Multiply(x, i);

    // If result is not 1 then it is not a multiplicative inverse
    if (fp61::Finalize(p) != 1)
    {
        cout << "Failed (finalized result not 1) for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    // Double check the reduce function...
    if (p % fp61::kPrime != 1)
    {
        cout << "Failed (remainder not 1) for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    return true;
}

static bool TestMulInverse()
{
    cout << "TestMulInverse...";

    // x < p

    // Small values
    for (uint64_t x = 1; x < 1000; ++x) {
        if (!test_inv(x)) {
            return false;
        }
    }

    fp61::Random prng;
    prng.Seed(5);

    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next();

        if (!test_inv(x)) {
            return false;
        }
    }

    cout << "Passed" << endl;

    return true;
}


//------------------------------------------------------------------------------
// Tests: ByteReader

bool test_byte_reader(const uint8_t* data, unsigned bytes)
{
    fp61::ByteReader reader;

    reader.BeginRead(data, bytes);

    // Round up to the next 61 bits
    uint64_t expandedBits = bytes * 8;
    unsigned actualReads = 0;
    unsigned bits = 0;
    bool packed = false;
    unsigned packedBit = 0;

    uint64_t fp;
    while (fp61::ReadResult::Success == reader.Read(fp))
    {
        unsigned readStart = bits / 8;
        if (readStart >= bytes)
        {
            // We can read one extra bit if the packing is the last thing
            if (!packed || readStart != bytes)
            {
                FP61_DEBUG_BREAK();
                cout << "Failed (too many reads) for bytes=" << bytes << " actualReads=" << actualReads << endl;
                return false;
            }
        }

        int readBytes = (int)bytes - (int)readStart;
        if (readBytes < 0) {
            readBytes = 0;
        }
        else if (readBytes > 8) {
            readBytes = 8;
        }

        uint64_t x = fp61::ReadBytes_LE(data + readStart, readBytes) >> (bits % 8);

        int readBits = (readBytes * 8) - (bits % 8);
        if (readBytes >= 8 && readBits > 0 && readBits < 61 && readStart + readBytes < bytes)
        {
            // Need to read one more byte sometimes
            uint64_t high = data[readStart + readBytes];
            high <<= readBits;
            x |= high;
        }

        // Test packing
        if (packed)
        {
            x <<= 1;
            x |= packedBit;
            bits += 60;
            ++expandedBits;
        }
        else
        {
            bits += 61;
        }

        x &= fp61::kPrime;

        packed = fp61::IsU64Ambiguous(x);
        if (packed)
        {
            packedBit = (x == fp61::kPrime);
            x = fp61::kAmbiguityMask;
        }

        if (fp != x)
        {
            FP61_DEBUG_BREAK();
            cout << "Failed (wrong value) for bytes=" << bytes << " actualReads=" << actualReads << endl;
            return false;
        }
        ++actualReads;
    }

    const unsigned expectedReads = (unsigned)((expandedBits + 60) / 61);
    if (actualReads != expectedReads)
    {
        FP61_DEBUG_BREAK();
        cout << "Failed (read count wrong) for bytes=" << bytes << endl;
        return false;
    }

    const unsigned maxWords = fp61::ByteReader::MaxWords(bytes);
    if (maxWords < actualReads)
    {
        FP61_DEBUG_BREAK();
        cout << "Failed (MaxWords wrong) for bytes=" << bytes << endl;
        return false;
    }

    return true;
}

bool TestByteReader()
{
    cout << "TestByteReader...";

    uint8_t data[10 + 8] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        0, 0, 0, 0, 0, 0, 0, 0 // Padding to simplify test
    };

    uint64_t w = fp61::ReadU64_LE(data);
    if (w != 0x0807060504030201ULL) {
        cout << "Failed (ReadU64_LE)" << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    uint32_t u = fp61::ReadU32_LE(data);
    if (u != 0x04030201UL) {
        cout << "Failed (ReadU32_LE)" << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    uint64_t z = fp61::ReadBytes_LE(data, 0);
    if (z != 0) {
        cout << "Failed (ReadBytes_LE 0)" << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    for (unsigned i = 1; i <= 8; ++i)
    {
        uint64_t v = fp61::ReadBytes_LE(data, i);
        uint64_t d = v ^ w;
        d <<= 8 * (8 - i);
        if (d != 0) {
            cout << "Failed (ReadBytes_LE) for i = " << i << endl;
            FP61_DEBUG_BREAK();
            return false;
        }
    }

    uint8_t simpledata[16 + 8] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        8, 9, 10, 11, 12, 13, 14, 15,
        0
    };

    for (unsigned i = 0; i <= 16; ++i)
    {
        if (!test_byte_reader(simpledata, i)) {
            return false;
        }
    }

    uint8_t allones[16 + 8] = {
        254,255,255,255,255,255,255,255,
        255,255,255,255,255,255,255,255,
        0
    };

    for (unsigned i = 0; i <= 16; ++i)
    {
        if (!test_byte_reader(allones, i)) {
            return false;
        }
    }

    uint8_t mixed[20 + 8] = {
        254,255,255,255,255,255,255,255,0, // Inject a non-overflowing bit in the middle
        255,255,255,255,255,255,255,
        255,255,255,255,
        0
    };

    for (unsigned i = 0; i <= 16; ++i)
    {
        if (!test_byte_reader(allones, i)) {
            return false;
        }
    }

    vector<uint8_t> randBytes(kMaxDataLength + 8, 0); // +8 to avoid bounds checking

    fp61::Random prng;
    prng.Seed(10);

    for (unsigned i = 0; i < kMaxDataLength; ++i)
    {
        for (unsigned j = 0; j < 1; ++j)
        {
            // Fill the data with random bytes
            for (unsigned k = 0; k < i; k += 8)
            {
                uint64_t w;
                if (prng.Next() % 100 <= 3) {
                    w = ~(uint64_t)0;
                }
                else {
                    w = prng.Next();
                }
                fp61::WriteU64_LE(&randBytes[k], w);
            }

            if (!test_byte_reader(&randBytes[0], i)) {
                return false;
            }
        }
    }

    cout << "Passed" << endl;

    return true;
}


//------------------------------------------------------------------------------
// Tests: Random

static bool TestRandom()
{
    cout << "TestRandom...";

    for (int i = -1000; i < 1000; ++i)
    {
        uint64_t loWord = static_cast<int64_t>(i);
        loWord <<= 3; // Put it in the high bits
        uint64_t loResult = fp61::Random::ConvertRandToFp(loWord);

        if (loResult >= fp61::kPrime)
        {
            cout << "Failed (RandToFp low) at i = " << i << endl;
            FP61_DEBUG_BREAK();
            return false;
        }

        uint64_t hiWord = fp61::kPrime + static_cast<int64_t>(i);
        hiWord <<= 3; // Put it in the high bits
        uint64_t hiResult = fp61::Random::ConvertRandToFp(hiWord);

        if (hiResult >= fp61::kPrime)
        {
            cout << "Failed (RandToFp high) at i = " << i << endl;
            FP61_DEBUG_BREAK();
            return false;
        }
    }

    for (int i = -1000; i < 1000; ++i)
    {
        uint64_t loWord = static_cast<int64_t>(i);
        loWord <<= 3; // Put it in the high bits
        uint64_t loResult = fp61::Random::ConvertRandToNonzeroFp(loWord);

        if (loResult <= 0 || loResult >= fp61::kPrime)
        {
            cout << "Failed (RandToNonzeroFp low) at i = " << i << endl;
            FP61_DEBUG_BREAK();
            return false;
        }

        uint64_t hiWord = fp61::kPrime + static_cast<int64_t>(i);
        hiWord <<= 3; // Put it in the high bits
        uint64_t hiResult = fp61::Random::ConvertRandToNonzeroFp(hiWord);

        if (hiResult <= 0 || hiResult >= fp61::kPrime)
        {
            cout << "Failed (RandToNonzeroFp high) at i = " << i << endl;
            FP61_DEBUG_BREAK();
            return false;
        }
    }

    cout << "Passed" << endl;

    return true;
}


//------------------------------------------------------------------------------
// Tests: WordReader/WordWriter

static bool TestWordSerialization()
{
    cout << "TestWordSerialization...";

    fp61::WordWriter writer;
    fp61::WordReader reader;

    fp61::Random prng;
    prng.Seed(11);

    std::vector<uint8_t> data;
    std::vector<uint64_t> wordData;

    for (unsigned i = 1; i < kMaxDataLength; ++i)
    {
        unsigned words = i;
        unsigned bytesNeeded = fp61::WordWriter::BytesNeeded(words);

        data.resize(bytesNeeded);
        wordData.resize(words);

        writer.BeginWrite(&data[0]);
        reader.BeginRead(&data[0], bytesNeeded);

        for (unsigned j = 0; j < words; ++j)
        {
            // Generate a value from 0..p because the writer technically does not care about staying within the field
            uint64_t w = prng.Next() & MASK61;
            wordData[j] = w;
            writer.Write(w);
        }
        writer.Flush();

        for (unsigned j = 0; j < words; ++j)
        {
            uint64_t u = reader.Read();
            if (u != wordData[j])
            {
                cout << "Failed (readback failed) at i = " << i << " j = " << j << endl;
                FP61_DEBUG_BREAK();
                return false;
            }
        }
    }

    cout << "Passed" << endl;

    return true;
}


//------------------------------------------------------------------------------
// Tests: ByteWriter

bool TestByteWriter()
{
    cout << "TestByteWriter...";

    fp61::ByteReader reader;
    fp61::ByteWriter writer;

    fp61::Random prng;
    prng.Seed(14);

    std::vector<uint8_t> original, recovered;

    for (unsigned i = 1; i < kMaxDataLength; ++i)
    {
        unsigned bytes = i;

        for (unsigned j = 0; j < 10; ++j)
        {
            // Padding to simplify tester
            original.resize(bytes + 8);

            // Fill the data with random bytes
            for (unsigned k = 0; k < i; k += 8)
            {
                uint64_t w;
                if (prng.Next() % 100 <= 3) {
                    w = ~(uint64_t)0;
                }
                else {
                    w = prng.Next();
                }
                fp61::WriteU64_LE(&original[k], w);
            }

            reader.BeginRead(&original[0], bytes);

            unsigned maxWords = fp61::ByteReader::MaxWords(bytes);
            unsigned maxBytes = fp61::ByteWriter::MaxBytesNeeded(maxWords);

            recovered.resize(maxBytes);
            writer.BeginWrite(&recovered[0]);

            // Write words we get directly back out
            uint64_t word;
            while (reader.Read(word) != fp61::ReadResult::Empty) {
                writer.Write(word);
            }
            unsigned writtenBytes = writer.Flush();

            // TBD: Check if high bits are 0?

            if (writtenBytes > maxBytes ||
                writtenBytes > bytes + 8)
            {
                cout << "Failed (byte count mismatch) at i = " << i << " j = " << j << endl;
                FP61_DEBUG_BREAK();
                return false;
            }

            if (0 != memcmp(&recovered[0], &original[0], bytes))
            {
                cout << "Failed (data corruption) at i = " << i << " j = " << j << endl;
                FP61_DEBUG_BREAK();
                return false;
            }
        }
    }

    cout << "Passed" << endl;

    return true;
}


//------------------------------------------------------------------------------
// Tests: Integration

// Tests all of the serialization/deserialization and some math code
bool TestIntegration()
{
    cout << "TestIntegration...";

    std::vector<uint8_t> data, recovery, recovered;

    fp61::Random prng;
    prng.Seed(13);

    // Test a range of data sizes
    for (unsigned i = 1; i < kMaxDataLength; ++i)
    {
        unsigned bytes = i;

        // Run a few tests for each size
        for (unsigned j = 0; j < 10; ++j)
        {
            // Generate some test data:

            // Allocate padded data to simplify tester
            data.resize(bytes + 8);

            // Fill the data with random bytes
            for (unsigned k = 0; k < i; k += 8)
            {
                uint64_t w;
                if (prng.Next() % 100 <= 3) {
                    w = ~(uint64_t)0;
                }
                else {
                    w = prng.Next();
                }
                fp61::WriteU64_LE(&data[k], w);
            }

            // Read data from the simulated packet,
            // perform some example Fp operation on it,
            // and then store it to a simulated recovery packet.

            // Preallocate enough space in recovery packets for the worst case
            const unsigned maxWords = fp61::ByteReader::MaxWords(bytes);
            recovery.resize(fp61::WordWriter::BytesNeeded(maxWords));

            fp61::WordWriter recovery_writer;
            recovery_writer.BeginWrite(&recovery[0]);

            fp61::ByteReader original_reader;
            original_reader.BeginRead(&data[0], bytes);

            fp61::Random coeff_prng;
            coeff_prng.Seed(bytes + j * 500000);

            // Start reading words from the original file/packet,
            // multiplying them by a random coefficient,
            // and writing them to the recovery file/packet.
            uint64_t r;
            while (original_reader.Read(r) == fp61::ReadResult::Success)
            {
                // Pick random coefficient to multiply between 1..p-1
                uint64_t coeff = coeff_prng.NextNonzeroFp();

                // x = r * coeff (62 bits)
                uint64_t x = fp61::Multiply(r, coeff);

                // Finalize x (61 bits < p)
                uint64_t f = fp61::Finalize(x);

                // Write to recovery file/packet
                recovery_writer.Write(f);
            }

            // Flush the remaining bits to the recovery file/packet
            unsigned writtenRecoveryBytes = recovery_writer.Flush();

            // Simulate reading data from the recovery file/packet
            // and recovering the original data:

            fp61::WordReader recovery_reader;
            recovery_reader.BeginRead(&recovery[0], writtenRecoveryBytes);

            // Allocate space for recovered data (may be up to 1.6% larger than needed)
            const unsigned recoveryWords = fp61::WordReader::WordCount(writtenRecoveryBytes);
            const unsigned maxBytes = fp61::ByteWriter::MaxBytesNeeded(recoveryWords);
            recovered.resize(maxBytes);

            fp61::ByteWriter original_writer;
            original_writer.BeginWrite(&recovered[0]);

            // Reproduce the same random sequence
            coeff_prng.Seed(bytes + j * 500000);

            // For each word to read:
            const unsigned readWords = fp61::WordReader::WordCount(writtenRecoveryBytes);
            for (unsigned i = 0; i < readWords; ++i)
            {
                // Pick random coefficient to multiply between 1..p-1
                uint64_t coeff = coeff_prng.NextNonzeroFp();
                uint64_t inv_coeff = fp61::Inverse(coeff);

                // Read the next word (61 bits)
                uint64_t f = recovery_reader.Read();

                // Invert the multiplication (62 bits)
                uint64_t x = fp61::Multiply(f, inv_coeff);

                // Finalize x (61 bits < p)
                x = fp61::Finalize(x);

                // Write to recovered original data buffer
                original_writer.Write(x);
            }

            // Flush the remaining bits to the recovered original file/packet
            unsigned recoveredBytes = original_writer.Flush();

            if (recoveredBytes > maxBytes ||
                recoveredBytes > bytes + 8)
            {
                cout << "Failed (byte count mismatch) at i = " << i << " j = " << j << endl;
                FP61_DEBUG_BREAK();
                return false;
            }

            if (0 != memcmp(&recovered[0], &data[0], bytes))
            {
                cout << "Failed (data corruption) at i = " << i << " j = " << j << endl;
                FP61_DEBUG_BREAK();
                return false;
            }
        }
    }

    cout << "Passed" << endl;

    return true;
}


//------------------------------------------------------------------------------
// Entrypoint

int main()
{
    cout << "Unit tester for Fp61.  Exits with -1 on failure, 0 on success" << endl;
    cout << endl;

    int result = FP61_RET_SUCCESS;

    if (!TestByteWriter()) {
        result = FP61_RET_FAIL;
    }
    if (!TestIntegration()) {
        result = FP61_RET_FAIL;
    }
    if (!TestRandom()) {
        result = FP61_RET_FAIL;
    }
    if (!TestWordSerialization()) {
        result = FP61_RET_FAIL;
    }
    if (!TestNegate()) {
        result = FP61_RET_FAIL;
    }
    if (!TestAdd()) {
        result = FP61_RET_FAIL;
    }
    if (!TestPartialReduction()) {
        result = FP61_RET_FAIL;
    }
    if (!TestFinalizeReduction()) {
        result = FP61_RET_FAIL;
    }
    if (!TestMultiply()) {
        result = FP61_RET_FAIL;
    }
    if (!TestMulInverse()) {
        result = FP61_RET_FAIL;
    }
    if (!TestByteReader()) {
        result = FP61_RET_FAIL;
    }

    cout << endl;
    if (result == FP61_RET_FAIL) {
        cout << "*** Tests failed (see above)!  Returning -1" << endl;
    }
    else {
        cout << "*** Tests succeeded!  Returning 0" << endl;
    }

    return result;
}
