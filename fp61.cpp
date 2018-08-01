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

#include "fp61.h"

namespace fp61 {


// This is an unrolled implementation of Knuth's unsigned version of the eGCD,
// specialized for the prime.  It handles any input.
uint64_t Inverse(uint64_t u)
{
    uint64_t u1, u3, v1, v3, qt;

    qt = u / kPrime;
    u3 = u % kPrime;
    u1 = 1;

    if (u3 == 0) {
        return 0; // No inverse
    }

    qt = kPrime / u3;
    v3 = kPrime % u3;
    v1 = qt;

    for (;;)
    {
        if (v3 == 0) {
            return u3 == 1 ? u1 : 0;
        }

        qt = u3 / v3;
        u3 %= v3;
        u1 += qt * v1;

        if (u3 == 0) {
            return v3 == 1 ? kPrime - v1 : 0;
        }

        qt = v3 / u3;
        v3 %= u3;
        v1 += qt * u1;
    }
}


//------------------------------------------------------------------------------
// Memory Reading

uint64_t ReadBytes_LE(const uint8_t* data, unsigned bytes)
{
    switch (bytes)
    {
    case 8: return ReadU64_LE(data);
    case 7: return ((uint64_t)data[6] << 48) | ((uint64_t)data[5] << 40) | ((uint64_t)data[4] << 32) | ReadU32_LE(data);
    case 6: return ((uint64_t)data[5] << 40) | ((uint64_t)data[4] << 32) | ReadU32_LE(data);
    case 5: return ((uint64_t)data[4] << 32) | ReadU32_LE(data);
    case 4: return ReadU32_LE(data);
    case 3: return ((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | data[0];
    case 2: return ((uint32_t)data[1] << 8) | data[0];
    case 1: return data[0];
    default: break;
    }
    return 0;
}

ReadResult ByteReader::ReadNext(uint64_t& fpOut)
{
    uint64_t word, r, workspace = Workspace;
    int nextAvailable, available = Available;

    // If enough bits are already available:
    if (available >= 61)
    {
        r = workspace & kPrime;
        workspace >>= 61;
        nextAvailable = available - 61;
    }
    else
    {
        unsigned bytes = Bytes;

        // Read a word to fill in the difference
        if (bytes >= 8)
        {
            word = ReadU64_LE(Data);
            Data += 8;
            Bytes = bytes - 8;
            nextAvailable = available + 3;
        }
        else
        {
            if (bytes == 0 && available <= 0) {
                return ReadResult::Empty;
            }

            word = ReadBytes_LE(Data, bytes);
            Bytes = 0;

            // Note this may go negative but we check for that above
            nextAvailable = available + bytes * 8 - 61;
        }

        // This assumes workspace high bits (beyond `available`) are 0
        r = (workspace | (word << available)) & kPrime;

        // Remaining workspace bits are taken from read word
        workspace = word >> (61 - available);
    }

    // If there is ambiguity in the representation:
    if (r >= kAmbiguity)
    {
        // This will not overflow because available <= 60.
        // We add up to 3 more bits, so adding one more keeps us within 64 bits.
        ++nextAvailable;

        // Insert bit 0 for fffe and 1 for ffff to resolve the ambiguity
        workspace = (workspace << 1) | (r & 1);

        // Use kAmbiguity value for a placeholder
        r = kAmbiguity;
    }

    Workspace = workspace;
    Available = nextAvailable;

    fpOut = r;
    return ReadResult::Success;
}

uint64_t WordReader::Read()
{
    int nextAvailable, available = Available;
    uint64_t r, workspace = Workspace;

    if (available >= 61)
    {
        r = workspace & kPrime;
        nextAvailable = available - 61;
        workspace >>= 61;
    }
    else
    {
        uint64_t word;
        unsigned bytes = Bytes;

        // If we can read a full word:
        if (bytes >= 8)
        {
            word = ReadU64_LE(Data);
            Data += 8;
            Bytes = bytes - 8;
            nextAvailable = available + 3; // +64 - 61
        }
        else
        {
            if (bytes == 0 && available <= 0) {
                return 0; // No data left to read
            }

            word = ReadBytes_LE(Data, bytes);

            // Note this may go negative but we check for negative above
            nextAvailable = available + bytes * 8 - 61;

            Bytes = 0;
        }

        r = workspace | (word << available);
        workspace = word >> (61 - available);
    }

    Workspace = workspace;
    Available = nextAvailable;

    return r;
}


//------------------------------------------------------------------------------
// Memory Writing

void WriteBytes_LE(uint8_t* data, unsigned bytes, uint64_t value)
{
    switch (bytes)
    {
    case 8: WriteU64_LE(data, value);
        return;
    case 7: data[6] = (uint8_t)(value >> 48);
    case 6: data[5] = (uint8_t)(value >> 40);
    case 5: data[4] = (uint8_t)(value >> 32);
    case 4: WriteU32_LE(data, static_cast<uint32_t>(value));
        return;
    case 3: data[2] = (uint8_t)(value >> 16);
    case 2: data[1] = (uint8_t)(value >> 8);
    case 1: data[0] = (uint8_t)value;
    default: break;
    }
}


//------------------------------------------------------------------------------
// Random

// From http://xoshiro.di.unimi.it/splitmix64.c
// Written in 2015 by Sebastiano Vigna (vigna@acm.org)
static uint64_t HashU64(uint64_t x)
{
    // Note there is an obvious weak key here that breaks the generator
    // TBD: Add something else after the muliply to fix this?
    x += 0x9e3779b97f4a7c15;
    uint64_t z = x;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

void Random::Seed(uint64_t x)
{
    // Fill initial state as recommended by authors
    uint64_t h = HashU64(x);
    State[0] = h;
    h = HashU64(h);
    State[1] = h;
    h = HashU64(h);
    State[2] = h;
    h = HashU64(h);
    State[3] = h;
}


} // namespace fp61
