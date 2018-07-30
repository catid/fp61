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

// This is an unrolled implementation of Knuth's unsigned version of the eGCD,
// specialized for the prime.  It handles any input.
uint64_t fp61_inv(uint64_t u)
{
    uint64_t u1, u3, v1, v3, qt;

    qt = u / kFp61Prime;
    u3 = u % kFp61Prime;
    u1 = 1;

    if (u3 == 0) {
        return 0; // No inverse
    }

    qt = kFp61Prime / u3;
    v3 = kFp61Prime % u3;
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
            return v3 == 1 ? kFp61Prime - v1 : 0;
        }

        qt = v3 / u3;
        v3 %= u3;
        v1 += qt * u1;
    }
}
