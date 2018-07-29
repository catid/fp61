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

uint64_t fp61_inv(uint64_t x)
{
    if (x <= 1) {
        return x;
    }

    int64_t s0, s1, r0, r1;

    // Compute the next remainder
    uint64_t uq = static_cast<uint64_t>(kFp61Prime) / x;

    // Store the results
    r0 = x;
    r1 = kFp61Prime - static_cast<int64_t>(uq * x);
    s0 = 1;
    s1 = -static_cast<int64_t>(uq);

    while (r1 != 1)
    {
        int64_t q, r, s;

        q = r0 / r1;
        r = r0 - (q * r1);
        s = s0 - (q * s1);

        r0 = r1;
        r1 = r;
        s0 = s1;
        s1 = s;
    }

    if (s1 < 0) {
        s1 += kFp61Prime;
    }

    return static_cast<uint64_t>(s1);
}
