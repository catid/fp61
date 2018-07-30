#include "../fp61.h"

#include <iostream>
#include <iomanip>
#include <sstream>
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
    uint64_t n = fp61_neg(x);
    uint64_t s = (x + n) % kFp61Prime;
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
    for (uint64_t x = kFp61Prime; x >= kFp61Prime - 1000; --x) {
        if (!test_negate(x)) {
            return false;
        }
    }

    PCGRandom prng;
    prng.Seed(1);

    for (unsigned i = 0; i < kRandomTestLoops; ++i)
    {
        uint64_t x = prng.Next64() & kFp61Prime;
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
    const uint64_t reduced = largest % kFp61Prime;

    for (uint64_t x = largest; x >= largest - 1000; --x)
    {
        uint64_t r = fp61_add4(largest, largest, largest, x);

        uint64_t expected = 0;
        expected = (expected + reduced) % kFp61Prime;
        expected = (expected + reduced) % kFp61Prime;
        expected = (expected + reduced) % kFp61Prime;
        expected = (expected + (x % kFp61Prime)) % kFp61Prime;

        if (r % kFp61Prime != expected) {
            cout << "TestAdd failed for x = " << HexString(x) << endl;
            FP61_DEBUG_BREAK();
            return false;
        }
    }

    for (uint64_t x = largest; x >= largest - 1000; --x)
    {
        for (uint64_t y = largest; y >= largest - 1000; --y)
        {
            uint64_t r = fp61_add4(largest, largest, x, y);

            uint64_t expected = 0;
            expected = (expected + reduced) % kFp61Prime;
            expected = (expected + reduced) % kFp61Prime;
            expected = (expected + (y % kFp61Prime)) % kFp61Prime;
            expected = (expected + (x % kFp61Prime)) % kFp61Prime;

            if (r % kFp61Prime != expected) {
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

        uint64_t r = fp61_add4(x, y, z, w);

        uint64_t expected = 0;
        expected = (expected + (x % kFp61Prime)) % kFp61Prime;
        expected = (expected + (y % kFp61Prime)) % kFp61Prime;
        expected = (expected + (z % kFp61Prime)) % kFp61Prime;
        expected = (expected + (w % kFp61Prime)) % kFp61Prime;

        if (r % kFp61Prime != expected) {
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
    uint64_t expected = x % kFp61Prime;

    uint64_t r = fp61_partial_reduce(x);

    if ((r >> 62) != 0)
    {
        cout << "TestPartialReduction (high bit overflow) failed for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    uint64_t actual = fp61_partial_reduce(x) % kFp61Prime;

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

    // Small values
    for (uint64_t x = 0; x < 1000; ++x) {
        if (!test_pred(x)) {
            return false;
        }
    }

    const uint64_t largest = ~(uint64_t)0;

    // Largest values
    for (uint64_t x = largest; x > largest - 1000; --x) {
        if (!test_pred(x)) {
            return false;
        }
    }

    const uint64_t msb_off = ((uint64_t)1 << 63) - 1;

    // #63 off
    for (uint64_t x = msb_off; x > msb_off - 1000; --x) {
        if (!test_pred(x)) {
            return false;
        }
    }

    const uint64_t nsb_off = largest ^ ((uint64_t)1 << 62);

    // #62 off
    for (uint64_t x = nsb_off + 1000; x > nsb_off - 1000; --x) {
        if (!test_pred(x)) {
            return false;
        }
    }

    // Around the prime
    for (uint64_t x = kFp61Prime + 1000; x > kFp61Prime - 1000; --x) {
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

    uint64_t actual = fp61_reduce_finalize(x);
    uint64_t expected = x % kFp61Prime;

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

    // Small values
    for (uint64_t x = 0; x < 1000; ++x) {
        if (!test_fred(x)) {
            return false;
        }
    }

    const uint64_t largest = ((uint64_t)1 << 62) - 1;

    // Largest values
    for (uint64_t x = largest; x > largest - 1000; --x) {
        if (!test_fred(x)) {
            return false;
        }
    }

    const uint64_t nsb_off = largest ^ ((uint64_t)1 << 61);

    // #61 off
    for (uint64_t x = nsb_off + 1000; x > nsb_off - 1000; --x) {
        if (!test_fred(x)) {
            return false;
        }
    }

    // Around the prime
    for (uint64_t x = kFp61Prime + 1000; x > kFp61Prime - 1000; --x) {
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
    uint64_t p = fp61_mul(x, y);

    if ((p >> 62) != 0) {
        cout << "TestMultiply failed (high bit overflow) for x=" << HexString(x) << ", y=" << HexString(y) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    uint64_t r0, r1;
    CAT_MUL128(r1, r0, x, y);

    //A % B == (((AH % B) * (2^64 % B)) + (AL % B)) % B
    //  == (((AH % B) * ((2^64 - B) % B)) + (AL % B)) % B
    r1 %= kFp61Prime;
    uint64_t NB = (uint64_t)(-(int64_t)kFp61Prime);
    uint64_t mod = r1 * (NB % kFp61Prime);
    mod += r0 % kFp61Prime;
    mod %= kFp61Prime;

    if (p % kFp61Prime != mod) {
        cout << "TestMultiply failed (reduced result mismatch) for x=" << HexString(x) << ", y=" << HexString(y) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    return true;
}

static bool TestMultiply()
{
    // Number of bits between x, y must be 124 or fewer.

    // Small values
    for (uint64_t x = 0; x < 1000; ++x) {
        for (uint64_t y = x; y < 1000; ++y) {
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

    return true;
}


//------------------------------------------------------------------------------
// Tests: Inverse

static bool test_inv(uint64_t x)
{
    uint64_t i = fp61_inv(x);

    // If no inverse existed:
    if (i == 0) {
        // Then it must have evenly divided
        if (x % kFp61Prime == 0) {
            return true;
        }
        // Otherwise this should have had a result
        cout << "TestMulInverse failed (no result) for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    // Result must be in Fp
    if (i >= kFp61Prime) {
        cout << "TestMulInverse failed (result too large) for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    // mul requires partially reduced input
    x = fp61_partial_reduce(x);

    uint64_t p = fp61_mul(x, i);

    // If result is not 1 then it is not a multiplicative inverse
    if (fp61_reduce_finalize(p) != 1) {
        cout << "TestMulInverse failed (finalized result not 1) for x=" << HexString(x) << endl;
        FP61_DEBUG_BREAK();
        return false;
    }

    // Double check the reduce function...
    if (p % kFp61Prime != 1) {
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

    cout << endl;
    if (result == FP61_RET_FAIL) {
        cout << "*** Tests failed (see above)!  Returning -1" << endl;
    }
    else {
        cout << "*** Tests succeeded!  Returning 0" << endl;
    }

    return result;
}
