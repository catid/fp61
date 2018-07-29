#include "../fp61.h"

#include <iostream>
using namespace std;

#define RET_FAIL -1
#define RET_SUCCESS 0

bool test_negate(uint64_t x)
{
    uint64_t n = fp61_neg(x);
    uint64_t s = (x + n) % kFp61Prime;
    if (s != 0) {
        cout << "TestNegate failed for x = " << x << endl;
        return false;
    }
    return true;
}

bool TestNegate()
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
    return true;
}

bool TestAdd()
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
            cout << "TestAdd failed for x = " << x << endl;
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
                cout << "TestAdd failed for x = " << x << endl;
                return false;
            }
        }
    }

    return true;
}

#define PR_TEST(x) \
    success &= (fp61_partial_reduce(x) % kFp61Prime) == ((x) % kFp61Prime);

bool TestPartialReduction()
{
    bool success = true;

    // Input can have any bit set

    // Small values
    for (uint64_t x = 0; x < 1000; ++x) {
        PR_TEST(x);
    }

    const uint64_t largest = ~(uint64_t)0;

    // Largest values
    for (uint64_t x = largest; x > largest - 1000; --x) {
        PR_TEST(x);
    }

    const uint64_t msb_off = ((uint64_t)1 << 63) - 1;

    // #63 off
    for (uint64_t x = msb_off; x > msb_off - 1000; --x) {
        PR_TEST(x);
    }

    const uint64_t nsb_off = largest ^ ((uint64_t)1 << 62);

    // #62 off
    for (uint64_t x = nsb_off + 1000; x > nsb_off - 1000; --x) {
        PR_TEST(x);
    }

    // Around the prime
    for (uint64_t x = kFp61Prime + 1000; x > kFp61Prime - 1000; --x) {
        PR_TEST(x);
    }

    if (!success) {
        cout << "TestPartialReduction failed" << endl;
    }

    return success;
}

#define FR_TEST(x) \
    success &= fp61_reduce_finalize(x) == ((x) % kFp61Prime);

bool TestFinalizeReduction()
{
    bool success = true;

    // Input has #63 and #62 clear, other bits can take on any value

    // Small values
    for (uint64_t x = 0; x < 1000; ++x) {
        FR_TEST(x);
    }

    const uint64_t largest = ((uint64_t)1 << 62) - 1;

    // Largest values
    for (uint64_t x = largest; x > largest - 1000; --x) {
        FR_TEST(x);
    }

    const uint64_t nsb_off = largest ^ ((uint64_t)1 << 61);

    // #61 off
    for (uint64_t x = nsb_off + 1000; x > nsb_off - 1000; --x) {
        FR_TEST(x);
    }

    // Around the prime
    for (uint64_t x = kFp61Prime + 1000; x > kFp61Prime - 1000; --x) {
        FR_TEST(x);
    }

    if (!success) {
        cout << "TestPartialReduction failed" << endl;
    }

    return success;
}

bool test_mul(uint64_t x, uint64_t y)
{
    uint64_t p = fp61_mul(x, y);

    if ((p >> 62) != 0) {
        cout << "TestMultiply failed (high bit overflow) for x=" << x << ", y=" << y << endl;
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
        cout << "TestMultiply failed (reduced result mismatch) for x=" << x << ", y=" << y << endl;
        return false;
    }

    return true;
}

bool TestMultiply()
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

    // TODO

    return true;
}

static bool test_inv(uint64_t x)
{
    uint64_t i = fp61_inv(x);
    if (i >= kFp61Prime) {
        cout << "TestMulInverse failed (result too large) for x=" << x << endl;
        return false;
    }
    uint64_t p = fp61_mul(x, i);
    if (fp61_reduce_finalize(p) != 1) {
        cout << "TestMulInverse failed (finalized result not 1) for x=" << x << endl;
        return false;
    }
    if (p % kFp61Prime != 1) {
        cout << "TestMulInverse failed (remainder not 1) for x=" << x << endl;
        return false;
    }
    return true;
}

bool TestMulInverse()
{
    // x < p

    // Small values
    for (uint64_t x = 1; x < 1000; ++x) {
        if (!test_inv(x)) {
            return false;
        }
    }

    // TODO

    return true;
}

int main()
{
    cout << "Unit tester for Fp61.  Exits with -1 on failure, 0 on success" << endl;

    int result = RET_SUCCESS;

    if (!TestNegate()) {
        result = RET_FAIL;
    }
    if (!TestAdd()) {
        result = RET_FAIL;
    }
    if (!TestPartialReduction()) {
        result = RET_FAIL;
    }
    if (!TestFinalizeReduction()) {
        result = RET_FAIL;
    }
    if (!TestMultiply()) {
        result = RET_FAIL;
    }
    if (!TestMulInverse()) {
        result = RET_FAIL;
    }

    return result;
}