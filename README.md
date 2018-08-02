# Fp61
## Finite field arithmetic modulo Mersenne prime p = 2^61-1 in C++

This software takes advantage of the commonly available fast 64x64->128 multiplier
to accelerate finite (base) field arithmetic.  So it runs a lot faster
when built into a 64-bit executable.

This math code offers use of lazy reduction techniques for speed,
via fp61::PartialReduce().

+ Addition of 8 values can be evaluated before reduction.
+ Sums of 4 products can be evaluated with partial reductions.

## API

Supported arithmetic operations: Add, Negation, Multiply, Mul Inverse.
Subtraction is implemented via Negation.

Partial Reduction from full 64 bits to 62 bits:

    r = fp61::PartialReduce(x)

    Partially reduce x (mod p).  This clears bits #63 and #62.

    The result can be passed directly to fp61::Add4(), fp61::Multiply(),
    and fp61::Finalize().

Final Reduction from 64 bits to <p (within the field Fp):

    r = fp61::Finalize(x)

    Finalize reduction of x (mod p) from PartialReduce()
    Preconditions: Bits #63 and #62 are clear and x != 0x3ffffffffffffffeULL

    This function fails for x = 0x3ffffffffffffffeULL.
    The partial reduction function does not produce this bit pattern for any
    input, so this exception is allowed because I'm assuming the input comes
    from fp61::PartialReduce().  So, do not mask down to 62 random bits and
    pass to this function because it can fail in this one case.

    Returns a value in Fp (less than p).

Chained Addition of Four Values:

    r = fp61::Add4(x, y, z, w)

    Sum x + y + z + w (without full reduction modulo p).
    Preconditions: x,y,z,w <2^62

    Probably you will want to just inline this code and follow the pattern,
    since being restricted to adding 4 things at a time is kind of weird.

    The result can be passed directly to fp61::Add4(), fp61::Multiply(), and
    fp61::Finalize().

You can also use normal addition but you have to be careful about bit overflow.

Negation:

    r = fp61::Negate(x)

    r = -x (without reduction modulo p)
    Preconditions: x <= p

    The input needs to be have bits #63 #62 #61 cleared.
    This can be ensured by calling fp61::PartialReduce() and
    fp61::Finalize() first.  Since this is more expensive than addition
    it is best to reorganize operations to avoid needing this reduction.

    Return a value <= p.

For subtraction, use fp61::Negate() and add: x + (-y).

Multiplication:

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

Modular Multiplicative Inverse:

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

Reading Byte Data (e.g. from a file or packet) Into 61-bit Field Words:

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

Writing Fp Words (e.g. storing field words to file or packet):

    WordWriter

    Writes a series of 61-bit finalized Fp field elements to a byte array.
    The resulting data can be read by WordReader.

    Call BytesNeeded() to calculate the number of bytes needed to store the
    given number of Fp words.

    Call BeginWrite() to start writing.
    Call Write() to write the next word.

    Call Flush() to write the last few bytes.
    Flush() returns the number of overall written bytes.

Reading Fp Words (e.g. from a file or packet):

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

Writing 61-bit Field Words Back Into Byte Data (e.g. recovering a file or packet):

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

Generating random Fp words (e.g. to fill a random matrix):

    Random

    Xoroshiro256+ based pseudo-random number generator (PRNG) that can generate
    random numbers between 1..p.  NextNonzeroFp() is mainly intended to be used
    for producing convolutional code coefficients to multiply by the data.

    Call Seed() to provide a 64-bit generator seed.
    Call NextNonzeroFp() to produce a random 61-bit number from 1..p
    Call NextFp() to produce a random 61-bit number from 0..p
    Call Next() to produce a random 64-bit number.


#### Comparing Fp61 to 8-bit and 16-bit Galois fields:

8-bit Galois field math requires SSSE3 instructions (PSHUFB) for best speed, which may not be available.  ARM64 and Intel both support the shuffle instruction.  It falls back to table lookups, which is relatively very slow.

16-bit Galois field math gets awkward and 2x slower to implement.  Awkward: The input data needs to be in 32 byte chunks for best speed, and interleaved in a special way.  Slower: Similar to schoolbook multiplication it requires 4 shuffles instead of 2 for the same amount of data because the base operation is limited to a 4-bit->8-bit lookup table (shuffle).

Fp61 math runs fastest when a 64x64->128 multiply instruction is available, which is unavailable on ARM64.  It has to use a schoolbook multiplication approach to emulate the wider multiplier, requiring 4 multiplies instead of 1.

Regarding fitting data into the fields, Galois fields have an advantage because input data is in bytes.  Data needs to be packed into Fp61 values in order to work on it, but the encoding is fairly straight-forward.

Regarding erasure code applications, a random linear code based on 8-bit Galois fields will fail to recover roughly 0.2% of the time, requiring one extra recovery packet.  16-bit Galois fields and Fp61 have almost no overhead.


#### Comparing Fp61 to Fp=2^32-5:

Fp61 has a number of advantages over Fp=2^32-5 and some disadvantages.

Clear advantages for Fp61:

(1) Since the prime modulus leaves some extra bits in the 64-bit words, lazy reduction can be used to cut down the cost of additions by about 1/2.  For erasure codes muladd/mulsub are the common operations, so cheap additions are great.

(2) The reductions are cheaper for Fp61 in general due to the Mersenne prime modulus.  Furthermore the reductions have no conditionals, so the performance is pretty much consistent regardless of the input.

(3) The smaller field consumes data at 1/2 the rate, which is problematic because of the data packing required.  Generally computers are more efficient for larger reads, so reading/writing twice as much data is more efficient.  If a prefix code is used, then 2x the amount of state needs to be kept for the same amount of data.  Fp61 must emit one bit for its prefix code, whereas the smaller field must emit 2-3 bits.

Possible advantages for Fp=2^32-5:

(1) Fp61 may be overall less efficient on mobile (ARM64) processors.  Despite the speed disadvantages discussed above, when the 64-bit multiply instruction is unavailable, the smaller prime may pull ahead for performance.  It would require benchmarking to really answer this.


#### Comparing Fp61 to Fp=2^127-1:

Perhaps Fp61 might be useful for cryptography for an extension field like Fp^4 for a 244-bit field.
All of the operations are constant-time so it is pretty close to being good for crypto.
The inverse operation would be exponentially faster since the Euler totient -based inverse function only needs to evaluate a 64-bit exponentiation instead of a 128-bit exponentiation.

Because the Fp=2^127-1 field is so close to the word size, every add operation needs a reduction costing 3 cycles.  The partial reduction for 2^61-1 runs in 2 cycles and only needs to be performed every 4 additions, plus it does not
require any assembly code software.
Overall addition for 4 values is 6x faster in this field.  If two 61-bit fields are used in an OEF, then addition of 61x2= 121-bit values is 3x faster using this as a base field.

Multiplication for 2^127-1 is complicated because there is no instruction that performs 128x128->256 bit products.  So it requires 4 MUL instructions. Reduction requires about 10 instructions.  Multiplication for 2^61-1 is done with 1 MUL instruction.
Reduction requires about 7 instructions for repeated muliplies.  In an OEF, the 121-bit multiply is overall less complicated, maybe by 30%?


#### Ideas for future work:

It may be interesting to use Fp=2^31-1 for mobile targets because the 32x32->64 multiplier that is available is a good fit for this field.  Reduction is simple and may allow for some laziness to cut the reduction costs in half, but it's not clear how it would work out practically without implementing it.


#### Credits

Software by Christopher A. Taylor <mrcatid@gmail.com>.

Please reach out if you need support or would like to collaborate on a project.
