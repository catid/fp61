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
#include "gf256.h"

#define FP61_ENABLE_GF256_COMPARE

/**
    Fp61 Benchmarks

    The goal of the benchmarks is to determine how fast Fp61 arithmetic is
    for the purpose of implementing erasure codes in software.


    *Drumroll...* Results:

    The results are not good at all.  The Fp61 encoder is roughly 20x slower
    than my Galois field code (gf256).  So, I do not recommend using Fp61.

    The majority of the slowdown comes from the ByteReader class that needs
    to convert byte data into 61-bit Fp words.  So it seems that having an
    odd field size to achieve lazy reductions does not help performance.

    *Sad trombone...*

    Benchmarks for Fp61 erasure codes.  Before running the benchmarks please run the tests to make sure everything's working on your PC.  It's going to run quite a bit faster with 64-bit builds because it takes advantage of the speed of 64-bit multiplications.

    Testing file size = 10 bytes
    N = 2 :  gf256_MBPS=250 Fp61_MBPS=65 Fp61_OutputBytes=16
    N = 4 :  gf256_MBPS=305 Fp61_MBPS=116 Fp61_OutputBytes=16
    N = 8 :  gf256_MBPS=138 Fp61_MBPS=80 Fp61_OutputBytes=16
    N = 16 :  gf256_MBPS=337 Fp61_MBPS=110 Fp61_OutputBytes=16
    N = 32 :  gf256_MBPS=711 Fp61_MBPS=242 Fp61_OutputBytes=16
    N = 64 :  gf256_MBPS=665 Fp61_MBPS=226 Fp61_OutputBytes=16
    N = 128 :  gf256_MBPS=868 Fp61_MBPS=297 Fp61_OutputBytes=16
    N = 256 :  gf256_MBPS=713 Fp61_MBPS=240 Fp61_OutputBytes=16
    N = 512 :  gf256_MBPS=881 Fp61_MBPS=300 Fp61_OutputBytes=16
    Testing file size = 100 bytes
    N = 2 :  gf256_MBPS=1234 Fp61_MBPS=214 Fp61_OutputBytes=107
    N = 4 :  gf256_MBPS=4000 Fp61_MBPS=486 Fp61_OutputBytes=107
    N = 8 :  gf256_MBPS=2631 Fp61_MBPS=328 Fp61_OutputBytes=107
    N = 16 :  gf256_MBPS=2051 Fp61_MBPS=300 Fp61_OutputBytes=107
    N = 32 :  gf256_MBPS=3850 Fp61_MBPS=433 Fp61_OutputBytes=107
    N = 64 :  gf256_MBPS=3972 Fp61_MBPS=428 Fp61_OutputBytes=107
    N = 128 :  gf256_MBPS=4397 Fp61_MBPS=444 Fp61_OutputBytes=107
    N = 256 :  gf256_MBPS=5137 Fp61_MBPS=500 Fp61_OutputBytes=107
    N = 512 :  gf256_MBPS=5129 Fp61_MBPS=492 Fp61_OutputBytes=107
    Testing file size = 1000 bytes
    N = 2 :  gf256_MBPS=10309 Fp61_MBPS=889 Fp61_OutputBytes=1007
    N = 4 :  gf256_MBPS=15325 Fp61_MBPS=848 Fp61_OutputBytes=1007
    N = 8 :  gf256_MBPS=9184 Fp61_MBPS=486 Fp61_OutputBytes=1007
    N = 16 :  gf256_MBPS=12728 Fp61_MBPS=722 Fp61_OutputBytes=1007
    N = 32 :  gf256_MBPS=11838 Fp61_MBPS=610 Fp61_OutputBytes=1007
    N = 64 :  gf256_MBPS=10555 Fp61_MBPS=604 Fp61_OutputBytes=1007
    N = 128 :  gf256_MBPS=11354 Fp61_MBPS=614 Fp61_OutputBytes=1007
    N = 256 :  gf256_MBPS=14782 Fp61_MBPS=816 Fp61_OutputBytes=1007
    N = 512 :  gf256_MBPS=18430 Fp61_MBPS=940 Fp61_OutputBytes=1007
    Testing file size = 10000 bytes
    N = 2 :  gf256_MBPS=19138 Fp61_MBPS=893 Fp61_OutputBytes=10004
    N = 4 :  gf256_MBPS=20283 Fp61_MBPS=959 Fp61_OutputBytes=10004
    N = 8 :  gf256_MBPS=20953 Fp61_MBPS=1010 Fp61_OutputBytes=10004
    N = 16 :  gf256_MBPS=22893 Fp61_MBPS=1056 Fp61_OutputBytes=10004
    N = 32 :  gf256_MBPS=24461 Fp61_MBPS=1087 Fp61_OutputBytes=10004
    N = 64 :  gf256_MBPS=22945 Fp61_MBPS=1057 Fp61_OutputBytes=10004
    N = 128 :  gf256_MBPS=16939 Fp61_MBPS=982 Fp61_OutputBytes=10004
    N = 256 :  gf256_MBPS=18608 Fp61_MBPS=927 Fp61_OutputBytes=10004
    N = 512 :  gf256_MBPS=16662 Fp61_MBPS=734 Fp61_OutputBytes=10004
    Testing file size = 100000 bytes
    N = 2 :  gf256_MBPS=22941 Fp61_MBPS=962 Fp61_OutputBytes=100002
    N = 4 :  gf256_MBPS=22827 Fp61_MBPS=976 Fp61_OutputBytes=100002
    N = 8 :  gf256_MBPS=16210 Fp61_MBPS=1052 Fp61_OutputBytes=100002
    N = 16 :  gf256_MBPS=17354 Fp61_MBPS=1044 Fp61_OutputBytes=100002
    N = 32 :  gf256_MBPS=16976 Fp61_MBPS=1030 Fp61_OutputBytes=100002
    N = 64 :  gf256_MBPS=13570 Fp61_MBPS=910 Fp61_OutputBytes=100002
    N = 128 :  gf256_MBPS=10592 Fp61_MBPS=533 Fp61_OutputBytes=100002
    N = 256 :  gf256_MBPS=10637 Fp61_MBPS=500 Fp61_OutputBytes=100002
    N = 512 :  gf256_MBPS=11528 Fp61_MBPS=483 Fp61_OutputBytes=100002


    Erasure codes are usually based on 8-bit Galois fields, but I was
    intrigued by the speed of the 64-bit multiplier on modern Intel processors.
    To take advantage of the fast multiplier I first looked at a number of field
    options before settling on Fp=2^61-1.  Note that I haven't benchmarked
    these other options so my comments might be misleading or incorrect.

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

    Reduction approaches considered:

    Montgomery:
    This requires that the Montgomery u factor has a low Hamming weight to
    implement efficiently.  p=2^64-2^32+1 happens to have this by chance,
    but it's a rare property.  It then requires two 128-bit products and adds.

    Pseudo-Mersenne:
    This does not require an efficient u factor, but still requires similarly
    two 128-bit products and adds.

    Mersenne:
    This is what Fp61 uses.  The reduction has to be applied multiple times to
    fully flush data back into the field < p, and it restricts the sizes of the
    inputs to 62 bits.  But in trade, no 128-bit operations are needed.
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

// Get maximum number of bytes needed for a recovery packet
static unsigned GetRecoveryBytes(unsigned originalBytes)
{
    const unsigned maxWords = fp61::ByteReader::MaxWords(originalBytes);
    const unsigned maxBytes = fp61::WordWriter::BytesNeeded(maxWords);
    return maxBytes;
}

/**
    Encode()

    This function implements the encoder for an erasure code.
    It accepts a set of equal-sized data packets and outputs one recovery packet
    that can repair one lost original packet.

    The recovery packet must be GetRecoveryBytes() in size.

    Returns the number of bytes written.
*/
unsigned Encode(
    const std::vector<std::vector<uint8_t>>& originals,
    unsigned N,
    unsigned bytes,
    uint64_t seed,
    uint8_t* recovery)
{
    uint64_t seedMix = fp61::HashU64(seed);

    std::vector<fp61::ByteReader> readers;
    readers.resize(N);
    for (unsigned i = 0; i < N; ++i) {
        readers[i].BeginRead(&originals[i][0], bytes);
    }

    fp61::WordWriter writer;
    writer.BeginWrite(recovery);

    const unsigned minWords = fp61::WordReader::WordCount(bytes);
    for (unsigned i = 0; i < minWords; ++i)
    {
        uint64_t fpword;
        readers[0].Read(fpword);
        uint64_t coeff = fp61::HashToNonzeroFp(seedMix + 0);
        uint64_t sum = fp61::Multiply(coeff, fpword);

        unsigned column = 1;
        unsigned columnsRemaining = N - 1;
        while (columnsRemaining >= 3)
        {
            uint64_t coeff0 = fp61::HashToNonzeroFp(seedMix + column);
            uint64_t coeff1 = fp61::HashToNonzeroFp(seedMix + column + 1);
            uint64_t coeff2 = fp61::HashToNonzeroFp(seedMix + column + 2);

            uint64_t fpword0, fpword1, fpword2;
            readers[column].Read(fpword0);
            readers[column + 1].Read(fpword1);
            readers[column + 2].Read(fpword2);

            sum += fp61::Multiply(coeff0, fpword0);
            sum += fp61::Multiply(coeff1, fpword1);
            sum += fp61::Multiply(coeff2, fpword2);
            sum = fp61::PartialReduce(sum);

            column += 3;
            columnsRemaining -= 3;
        }

        while (columnsRemaining > 0)
        {
            uint64_t temp;
            readers[column].Read(temp);
            sum += fp61::Multiply(coeff, temp);

            column++;
            columnsRemaining--;
        }
        sum = fp61::PartialReduce(sum);
        sum = fp61::Finalize(sum);
        writer.Write(sum);
    }

    for (;;)
    {
        bool more_data = false;

        uint64_t sum = 0;

        for (unsigned i = 0; i < N; ++i)
        {
            uint64_t coeff = fp61::HashToNonzeroFp(seedMix + i);

            uint64_t fpword;
            if (readers[i].Read(fpword) == fp61::ReadResult::Success)
            {
                more_data = true;

                sum += fp61::Multiply(coeff, fpword);
                sum = fp61::PartialReduce(sum);
            }
        }

        if (!more_data) {
            break;
        }

        sum = fp61::Finalize(sum);
        writer.Write(sum);
    }

    return writer.Flush();
}

void EncodeGF256(
    const std::vector<std::vector<uint8_t>>& originals,
    unsigned N,
    unsigned bytes,
    uint64_t seed,
    uint8_t* recovery)
{
    uint64_t seedMix = fp61::HashU64(seed);

    uint8_t coeff = (uint8_t)fp61::HashToNonzeroFp(seedMix + 0);
    if (coeff == 0) {
        coeff = 1;
    }

    gf256_mul_mem(recovery, &originals[0][0], coeff, bytes);

    for (unsigned i = 1; i < N; ++i)
    {
        coeff = (uint8_t)fp61::HashToNonzeroFp(seedMix + 0);
        if (coeff == 0) {
            coeff = 1;
        }

        gf256_muladd_mem(recovery, coeff, &originals[i][0], bytes);
    }
}


//------------------------------------------------------------------------------
// Benchmarks

static const unsigned kFileSizes[] = {
    10, 100, 1000, 10000, 100000
};
static const unsigned kFileSizesCount = static_cast<unsigned>(sizeof(kFileSizes) / sizeof(kFileSizes[0]));

static const unsigned kFileN[] = {
    2, 4, 8, 16, 32, 64, 128, 256, 512
};
static const unsigned kFileNCount = static_cast<unsigned>(sizeof(kFileN) / sizeof(kFileN[0]));

static const unsigned kTrials = 1000;

void RunBenchmarks()
{
    fp61::Random prng;
    prng.Seed(0);

    std::vector<std::vector<uint8_t>> original_data;
    std::vector<uint8_t> recovery_data;

    for (unsigned i = 0; i < kFileSizesCount; ++i)
    {
        unsigned fileSizeBytes = kFileSizes[i];

        cout << "Testing file size = " << fileSizeBytes << " bytes" << endl;

        for (unsigned j = 0; j < kFileNCount; ++j)
        {
            unsigned N = kFileN[j];

            cout << "N = " << N << " : ";

            uint64_t sizeSum = 0, timeSum = 0;
            uint64_t timeSum_gf256 = 0;

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

                    Lazy reduction can be used to simplify the add steps.

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

                original_data.resize(N);
                for (unsigned s = 0; s < N; ++s)
                {
                    // Add 8 bytes padding to simplify tester
                    original_data[s].resize(fileSizeBytes + 8);

                    // Fill the data with random bytes
                    for (unsigned r = 0; r < i; r += 8)
                    {
                        uint64_t w;
                        if (prng.Next() % 100 <= 3) {
                            w = ~(uint64_t)0;
                        }
                        else {
                            w = prng.Next();
                        }
                        fp61::WriteU64_LE(&original_data[s][r], w);
                    }
                }

                const unsigned maxRecoveryBytes = GetRecoveryBytes(fileSizeBytes);
                recovery_data.resize(maxRecoveryBytes);

                {
                    uint64_t t0 = GetTimeUsec();

                    unsigned recoveryBytes = Encode(original_data, N, fileSizeBytes, k, &recovery_data[0]);

                    uint64_t t1 = GetTimeUsec();

                    sizeSum += recoveryBytes;
                    timeSum += t1 - t0;
                }

#ifdef FP61_ENABLE_GF256_COMPARE
                {
                    uint64_t t0 = GetTimeUsec();

                    EncodeGF256(original_data, N, fileSizeBytes, k, &recovery_data[0]);

                    uint64_t t1 = GetTimeUsec();

                    timeSum_gf256 += t1 - t0;
                }
#endif // FP61_ENABLE_GF256_COMPARE
            }

#ifdef FP61_ENABLE_GF256_COMPARE
            cout << " gf256_MBPS=" << (uint64_t)fileSizeBytes * N * kTrials / timeSum_gf256;
#endif // FP61_ENABLE_GF256_COMPARE
            cout << " Fp61_MBPS=" << (uint64_t)fileSizeBytes * N * kTrials / timeSum;
            cout << " Fp61_OutputBytes=" << sizeSum / (float)kTrials;
            cout << endl;
        }
    }
}


//------------------------------------------------------------------------------
// Entrypoint

int main()
{
    cout << "Benchmarks for Fp61 erasure codes.  Before running the benchmarks please run the tests to make sure everything's working on your PC.  It's going to run quite a bit faster with 64-bit builds because it takes advantage of the speed of 64-bit multiplications." << endl;
    cout << endl;

    gf256_init();

    RunBenchmarks();

    cout << endl;
    return 0;
}
