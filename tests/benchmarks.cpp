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

#include "../fp61.h"

/**
    Fp61 Benchmarks

    The goal of the benchmarks is to determine how fast Fp61 arithmetic is
    for the purpose of implementing erasure codes in software.

    The usual implementation is based on 8-bit Galois fields, but I was
    intrigued by the speed of the 64-bit multiplier on modern Intel processors.
    To take advantage of the fast multiplier I first looked at a number of field
    options before settling on Fp=2^61-1.

    Some other options I investigated:

    Fp=2^64+c
    - The values of `c` that I found had a high Hamming weight so would be
      expensive to reduce using the pseudo-Mersenne reduction approach.
    - These seem to be patented.  Didn't really look into that issue.
    - Fp values do not fit into 64-bit words so they're slower to work with.
    - The reduction seems to require 128-bit adds/subs to implement properly,
      which are awkward to implement on some compilers.
    - There's no room for lazy reductions, so adds/subs are more expensive.

    Fp=2^64-c, specifically Solinas prime Fp=2^64-2^8-1
    - The smallest values of `c` that I found had a high Hamming weight so would
      be expensive to reduce using the pseudo-Mersenne reduction approach.
    - The reduction seems to require 128-bit adds/subs to implement properly,
      which are awkward to implement on some compilers.
    - There's no room for lazy reductions, so adds/subs are more expensive.
    ? Packing might be a littler simpler since all data is word-sized ?

    Fp=2^64-2^32+1
    - Honestly I tried to get the Solinas prime reduction to work and failed
      to implement it properly. See my notes below.
    - There's no room for lazy reductions, so adds/subs are more expensive.
    ? Packing might be a littler simpler since all data is word-sized ?
    + If it works the way I expect, then it could be even faster than Fp61.
*/

/**
    Fp=2^64-2^32+1 Solinas prime reduction (work in progress)

    [1] Reduction as in "Generalized Mersenne Numbers" J. Solinas (NSA).
    [2] Prime from "Solinas primes of small weight for fixed sizes" J. Angel.

    Fp = 2^64-2^32+1 from [2]

    (Borrowing notation from [1])
    t = 2^32
    d = 2
    f(t) = t^2 - c_1 * t - c_2

    c_1 = 1
    c_2 = -1

    X_0j = c_(d-j)
    X_00 = c_2
    x_01 = c_1

    X_i0 = X_(i-1,d-1) * c_d
    X_ij = X_(i-1,j-1) + X_(i-1,d-1) * c_(d-j)

    X_00 = -1
    x_01 = 1

    X_i0 = -X_(i-1,1)
    X_i1 = X_(i-1,0) + X_(i-1,1)

    X_10 = -1
    X_11 = 0

    [ t^2 t^3 ] = [ [-1 1] [-1 0] ] * [1 t] (mod f(t))

    [A0 A1] [1 t] + [A2 A3] [t^2 t^3]
    = ([A0 A1] + [A2 A3] * [[-1 1][-1 0]]) * [1 t]

    64-bit result B1 | B0:

    B0 = A0 - A2 + A3 (32-bit values)
    B1 = A1 - A2      (32-bit values)

    And then I tried to implement this and failed.
    Maybe I didn't follow the recipe properly?
    If this actually works then it seems like a pretty kick-ass
    alternative to Fp61 even if it does not support lazy reductions.
*/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
using namespace std;


#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#elif __MACH__
    #include <mach/mach_time.h>
    #include <mach/mach.h>
    #include <mach/clock.h>

    extern mach_port_t clock_port;
#else
    #include <time.h>
    #include <sys/time.h>
#endif


//------------------------------------------------------------------------------
// Timing

#ifdef _WIN32
// Precomputed frequency inverse
static double PerfFrequencyInverseUsec = 0.;
static double PerfFrequencyInverseMsec = 0.;

static void InitPerfFrequencyInverse()
{
    LARGE_INTEGER freq = {};
    if (!::QueryPerformanceFrequency(&freq) || freq.QuadPart == 0)
        return;
    const double invFreq = 1. / (double)freq.QuadPart;
    PerfFrequencyInverseUsec = 1000000. * invFreq;
    PerfFrequencyInverseMsec = 1000. * invFreq;
}
#elif __MACH__
static bool m_clock_serv_init = false;
static clock_serv_t m_clock_serv = 0;

static void InitClockServ()
{
    m_clock_serv_init = true;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &m_clock_serv);
}
#endif // _WIN32

uint64_t GetTimeUsec()
{
#ifdef _WIN32
    LARGE_INTEGER timeStamp = {};
    if (!::QueryPerformanceCounter(&timeStamp))
        return 0;
    if (PerfFrequencyInverseUsec == 0.)
        InitPerfFrequencyInverse();
    return (uint64_t)(PerfFrequencyInverseUsec * timeStamp.QuadPart);
#elif __MACH__
    if (!m_clock_serv_init)
        InitClockServ();

    mach_timespec_t tv;
    clock_get_time(m_clock_serv, &tv);

    return 1000000 * tv.tv_sec + tv.tv_nsec / 1000;
#else
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return 1000000 * tv.tv_sec + tv.tv_usec;
#endif
}


//------------------------------------------------------------------------------
// Fp61 Erasure Code Encoder

/**
    Encode()

    This function implements the encoder for an erasure code.
    It accepts a set of equal-sized data packets and outputs one recovery packet
    that can repair one lost original packet.
*/
void Encode(
    const void**data,
    unsigned bytes,
    uint64_t seed)
{

}


//------------------------------------------------------------------------------
// Benchmarks

static const unsigned kFileSizes[] = {
    10, 100, 1000, 10000
};
static const unsigned kFileSizesCount = static_cast<unsigned>(sizeof(kFileSizes) / sizeof(kFileSizes[0]));

static const unsigned kFileN[] = {
    2, 4, 8, 16, 32, 64, 128, 256
};
static const unsigned kFileNCount = static_cast<unsigned>(sizeof(kFileN) / sizeof(kFileN[0]));

static const unsigned kTrials = 100;

void RunBenchmarks()
{
    fp61::Random prng;
    prng.Seed(0);

    for (unsigned i = 0; i < kFileSizesCount; ++i)
    {
        unsigned fileSize = kFileSizes[i];

        cout << "Testing file size = " << fileSize << " bytes" << endl;

        for (unsigned j = 0; j < kFileNCount; ++j)
        {
            unsigned N = kFileN[j];

            cout << "N = " << N << " : ";

            for (unsigned k = 0; k < kTrials; ++k)
            {
                /*
                    File pieces: f0, f1, f3, f4, ...
                    Coefficients: m0, m1, m2, m3, ...

                    R = m0 * f0 + m1 * f1 + m2 * f2 + ...

                    R = sum(m_i * f_i) (mod 2^61-1)

                    To compute the recovery packet R we process the calculations
                    for the first word from all of the file pieces to produce a
                    single word of output.  This is a matrix-vector product
                    between file data f_i (treated as Fp words) and randomly
                    chosen generator matrix coefficients m_i.

                    Then we continue to the next word for all the file pieces,
                    producing the next word of output.

                    It is possible to interleave the calculations for output
                    words, and for input words to achieve higher throughput.

                    The number of words for each file piece can vary slightly
                    based on the data (if the data bytes do not fit evenly into
                    the Fp words, we have to add extra bits to resolve
                    ambiguities).

                    The result is a set of 61-bit Fp words serialized to bytes,
                    that is about 8 bytes more than the original file sizes.

                    The erasure code decoder (not implemented) would be able
                    to take these recovery packets and fix lost data.
                    The decoder performance would be fairly similar to the
                    encoder performance for this type of erasure code, since
                    the runtime is dominated by this matrix-vector product.
                */

                fp61::Random matrix_generator;
                matrix_generator.Seed(prng.Next());

                // TODO
            }
        }
    }
}


//------------------------------------------------------------------------------
// Entrypoint

int main()
{
    cout << "Benchmarks for Fp61 erasure codes.  Before running the benchmarks please run the tests to make sure everything's working on your PC.  It's going to run quite a bit faster with 64-bit builds because it takes advantage of the speed of 64-bit multiplications." << endl;
    cout << endl;

    RunBenchmarks();

    cout << endl;
    return 0;
}
