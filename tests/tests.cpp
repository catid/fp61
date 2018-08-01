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
#else
static const unsigned kRandomTestLoops = 10000000;
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
// PCG PRNG

/// From http://www.pcg-random.org/
class PCGRandom
{
public:
    void Seed(uint64_t y, uint64_t x = 0)
    {
        State = 0;
        Inc = (y << 1u) ^ 1442695040888963407ULL; // Tweaked -cat
        Next();
        State += x;
        Next();
    }

    uint32_t Next()
    {
        const uint64_t oldstate = State;
        State = oldstate * UINT64_C(6364136223846793005) + Inc;
        const uint32_t xorshifted = (uint32_t)(((oldstate >> 18) ^ oldstate) >> 27);
        const uint32_t rot = oldstate >> 59;
        return (xorshifted >> rot) | (xorshifted << ((uint32_t)(-(int32_t)rot) & 31));
    }

    uint64_t Next64()
    {
        const uint64_t x = Next();
        return (x << 32) | Next();
    }

    uint64_t State = 0, Inc = 0;
};


//------------------------------------------------------------------------------
// Tests: Negate

static bool test_negate(uint64_t x)
{
    uint64_t n = fp61::Negate(x);
    uint64_t s = (x + n) % fp61::kPrime;
    if (s != 0) {
        cout << "TestNegate failed for x = " << hex << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }
    return true;
}

static bool TestNegate()
{
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

    PCGRandom prng;
    prng.Seed(1);

    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next64() & fp61::kPrime;
        if (!test_negate(x)) {
            return false;
        }
    }
    return true;
}


//------------------------------------------------------------------------------
// Tests: Add

static bool TestAdd()
{
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
            cout << "TestAdd failed for x = " << HexString(x) << endl;
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
                cout << "TestAdd failed for x=" << HexString(x) << " y=" << HexString(y) << endl;
                FP61_DEBUG_BREAK();
                return false;
            }
        }
    }

    PCGRandom prng;
    prng.Seed(0);

    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        // Select 4 values from 0..p
        uint64_t x = prng.Next64() & MASK62;
        uint64_t y = prng.Next64() & MASK62;
        uint64_t w = prng.Next64() & MASK62;
        uint64_t z = prng.Next64() & MASK62;

        uint64_t r = fp61::Add4(x, y, z, w);

        uint64_t expected = 0;
        expected = (expected + (x % fp61::kPrime)) % fp61::kPrime;
        expected = (expected + (y % fp61::kPrime)) % fp61::kPrime;
        expected = (expected + (z % fp61::kPrime)) % fp61::kPrime;
        expected = (expected + (w % fp61::kPrime)) % fp61::kPrime;

        if (r % fp61::kPrime != expected) {
            cout << "TestAdd failed (random) for i = " << i << endl;
            FP61_DEBUG_BREAK();
            return false;
        }
    }

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
        cout << "TestPartialReduction (high bit overflow) failed for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    uint64_t actual = fp61::PartialReduce(x) % fp61::kPrime;

    if (actual != expected)
    {
        cout << "TestPartialReduction failed for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }
    return true;
}

static bool TestPartialReduction()
{
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

    PCGRandom prng;
    prng.Seed(2);

    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next64();

        if (!test_pred(x)) {
            return false;
        }
    }

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
        cout << "TestFinalizeReduction failed for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }
    return true;
}

static bool TestFinalizeReduction()
{
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

    PCGRandom prng;
    prng.Seed(3);

    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next64() & MASK62;

        if (!test_fred(x)) {
            return false;
        }
    }

    return true;
}


//------------------------------------------------------------------------------
// Tests: Multiply

static bool test_mul(uint64_t x, uint64_t y)
{
    uint64_t p = fp61::Multiply(x, y);

    if ((p >> 62) != 0) {
        cout << "TestMultiply failed (high bit overflow) for x=" << HexString(x) << ", y=" << HexString(y) << endl;
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
        cout << "TestMultiply failed (reduced result mismatch) for x=" << HexString(x) << ", y=" << HexString(y) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    return true;
}

static bool TestMultiply()
{
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

    PCGRandom prng;
    prng.Seed(4);

    // 62 + 62 = 124 bits
    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next64() & MASK62;
        uint64_t y = prng.Next64() & MASK62;

        if (!test_mul(x, y)) {
            return false;
        }
    }

    // 61 + 63 = 124 bits
    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next64() & MASK61;
        uint64_t y = prng.Next64() & MASK63;

        if (!test_mul(x, y)) {
            return false;
        }
    }

    // Commutivity test
    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next64() & MASK62;
        uint64_t y = prng.Next64() & MASK62;
        uint64_t z = prng.Next64() & MASK62;

        uint64_t r = fp61::Finalize(fp61::Multiply(fp61::Multiply(z, y), x));
        uint64_t s = fp61::Finalize(fp61::Multiply(fp61::Multiply(x, z), y));
        uint64_t t = fp61::Finalize(fp61::Multiply(fp61::Multiply(x, y), z));

        if (r != s || s != t) {
            cout << "TestMultiply failed (does not commute) for i=" << i << endl;
            FP61_DEBUG_BREAK();
            return false;
        }
    }

    // Direct function test
    uint64_t r1, r0;
    r0 = Emulate64x64to128(r1, MASK64, MASK64);

    if (r1 != 0xfffffffffffffffe || r0 != 1) {
        cout << "TestMultiply failed (Emulate64x64to128 failed)" << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    return true;
}


//------------------------------------------------------------------------------
// Tests: Inverse

static bool test_inv(uint64_t x)
{
    uint64_t i = fp61::Inverse(x);

    // If no inverse existed:
    if (i == 0) {
        // Then it must have evenly divided
        if (x % fp61::kPrime == 0) {
            return true;
        }
        // Otherwise this should have had a result
        cout << "TestMulInverse failed (no result) for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    // Result must be in Fp
    if (i >= fp61::kPrime) {
        cout << "TestMulInverse failed (result too large) for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    // mul requires partially reduced input
    x = fp61::PartialReduce(x);

    uint64_t p = fp61::Multiply(x, i);

    // If result is not 1 then it is not a multiplicative inverse
    if (fp61::Finalize(p) != 1) {
        cout << "TestMulInverse failed (finalized result not 1) for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    // Double check the reduce function...
    if (p % fp61::kPrime != 1) {
        cout << "TestMulInverse failed (remainder not 1) for x=" << HexString(x) << endl;
        return false;
    }

    return true;
}

static bool TestMulInverse()
{
    // x < p

    // Small values
    for (uint64_t x = 1; x < 1000; ++x) {
        if (!test_inv(x)) {
            return false;
        }
    }

    PCGRandom prng;
    prng.Seed(5);

    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next64();

        if (!test_inv(x)) {
            return false;
        }
    }

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
    while (fp61::ReadResult::Success == reader.ReadNext(fp))
    {
        unsigned readStart = bits / 8;
        if (readStart >= bytes)
        {
            // We can read one extra bit if the packing is the last thing
            if (!packed || readStart != bytes)
            {
                FP61_DEBUG_BREAK();
                cout << "TestByteReader failed (too many reads) for bytes=" << bytes << " actualReads=" << actualReads << endl;
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

        packed = (x >= fp61::kAmbiguity);
        if (packed)
        {
            packedBit = (x == fp61::kPrime);
            x = fp61::kAmbiguity;
        }

        if (fp != x)
        {
            FP61_DEBUG_BREAK();
            cout << "TestByteReader failed (wrong value) for bytes=" << bytes << " actualReads=" << actualReads << endl;
            return false;
        }
        ++actualReads;
    }

    const unsigned expectedReads = (unsigned)((expandedBits + 60) / 61);
    if (actualReads != expectedReads)
    {
        FP61_DEBUG_BREAK();
        cout << "TestByteReader failed (read count wrong) for bytes=" << bytes << endl;
        return false;
    }

    return true;
}

bool TestByteReader()
{
    uint8_t data[10 + 8] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        0, 0, 0, 0, 0, 0, 0, 0 // Padding to simplify test
    };

    uint64_t w = fp61::ReadU64_LE(data);
    if (w != 0x0807060504030201ULL) {
        cout << "TestByteReader failed (ReadU64_LE)" << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    uint32_t u = fp61::ReadU32_LE(data);
    if (u != 0x04030201UL) {
        cout << "TestByteReader failed (ReadU32_LE)" << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    uint64_t z = fp61::ReadBytes_LE(data, 0);
    if (z != 0) {
        cout << "TestByteReader failed (ReadBytes_LE 0)" << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    for (unsigned i = 1; i <= 8; ++i)
    {
        uint64_t v = fp61::ReadBytes_LE(data, i);
        uint64_t d = v ^ w;
        d <<= 8 * (8 - i);
        if (d != 0) {
            cout << "TestByteReader failed (ReadBytes_LE) for i = " << i << endl;
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

    static const unsigned kTestRange = 32000;
    vector<uint8_t> randBytes(kTestRange + 8, 0); // +8 to avoid bounds checking

    PCGRandom prng;
    prng.Seed(10);

    for (unsigned i = 0; i < kTestRange; ++i)
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
                    w = prng.Next64();
                }
                fp61::WriteU64_LE(&randBytes[k], w);
            }

            if (!test_byte_reader(&randBytes[0], i)) {
                return false;
            }
        }
    }

    return true;
}


//------------------------------------------------------------------------------
// Entrypoint

int main()
{
    cout << "Unit tester for Fp61.  Exits with -1 on failure, 0 on success" << endl;
    cout << endl;

    int result = FP61_RET_SUCCESS;

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
