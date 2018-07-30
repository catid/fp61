# Fp61
## Finite field arithmetic modulo 2^61-1

This software implements arithmetic modulo the Mersenne prime p = 2^61-1.

It takes advantage of the commonly available fast 64x64->128 multiplier
to accelerate finite (base) field arithmetic.  So it runs a lot faster
when built into a 64-bit executable.

This math code offers full use of lazy reduction techniques,
via fp61_partial_reduce().
+ Addition of 8 values can be evaluated before reduction.
+ Sums of 4 products can be evaluated with partial reductions.

It is for the most part constant-time, though not intended for cryptography.
I wrote the library to benchmark this field for software erasure codes.

## API

Supported arithmetic operations: Add, Negation, Multiply, Mul Inverse.
Subtraction is implemented via Negation.

Partial Reduction from full 64 bits to 62 bits:

    fp61_partial_reduce()

    Partially reduce a value (mod p).  This clears bits #63 and #62.

    The result can be passed directly to fp61_add4(), fp61_mul(),
    and fp61_reduce_finalize().

Final Reduction from 64 bits to <p (within the field Fp):

    fp61_reduce_finalize()

    Finalize reduction of a value (mod p) that was partially reduced
    Preconditions: Bits #63 and #62 are clear and x != 0x3ffffffffffffffeULL

    This function fails for x = 0x3ffffffffffffffeULL.
    The partial reduction function does not produce this bit pattern for any
    input, so this exception is allowed because I'm assuming the input comes
    from fp61_partial_reduce().  So, do not mask down to 62 random bits and
    pass to this function because it can fail in this one case.

    Returns a value less than p.

Chained Addition of Four Values:

    fp61_add4()

    Sum x + y + z + w (without full reduction modulo p).
    Preconditions: x,y,z,w <2^62

    Probably you will want to just inline this code and follow the pattern,
    since being restricted to adding 4 things at a time is kind of weird.

    The result can be passed directly to fp61_add4(), fp61_mul(), and
    fp61_reduce_finalize().

You can also use normal addition but you have to be careful about bit overflow.

Negation:

    r = fp61_neg(x)

    r = -x (without reduction modulo p)
    Preconditions: x <= p

    The input needs to be have bits #63 #62 #61 cleared.
    This can be ensured by calling fp61_partial_reduce() and
    fp61_reduce_finalize() first.  Since this is more expensive than addition
    it is best to reorganize operations to avoid needing this reduction.

    Return a value <= p.

For subtraction, use fp61_neg() and fp61_add4().
As in: x + (-y)

Multiplication:

    r = fp61_mul(x, y)

    r = x * y (with partial reduction modulo p)

    Important Input Restriction:

        The number of bits between x and y must be less than 124 bits.

        Call fp61_partial_reduce() to reduce inputs if needed,
        which makes sure that both inputs are 62 bits or fewer.

        Example: If x <= 2^62-1 (62 bits), then y <= 2^62-1 (62 bits).
        This means that up to 2 values can be accumulated in x and 2 in y.

        But it is also possible to balance the input in other ways.

        Example: If x <= 2^61-1 (61 bits), then y <= 2^63-1 (63 bits).
        This means that up to 4 values can be accumulated in y.

    Result:

        The result is stored in bits #61 to #0 (62 bits of the word).
        Call fp61_final_reduce() to reduce the result to 61 bits.

Modular Multiplicative Inverse:

    r = fp61_inv(x)

    r = x^-1 (mod p)
    The input value x can be any 64-bit value.

    This operation is kind of heavy so it should be avoided where possible.

    This operation is not constant-time.
    A constant-time version can be implemented using Euler's totient method and
    a straight line similar to https://github.com/catid/snowshoe/blob/master/src/fp.inc#L545

    Returns the multiplicative inverse of x modulo p.
    0 < result < p

    If the inverse does not exist, it returns 0.


#### Compare to Fp=2^127-1:

Because the field is so close to the word size, every add operation needs
a reduction costing 3 cycles.  The partial reduction for 2^61-1 runs in 2
cycles and only needs to be performed every 4 additions, plus it does not
require any assembly code software.
Overall addition for 4 values is 6x faster in this field.  If two 61-bit
fields are used in an OEF, then addition of 61*2= 121-bit values is 3x
faster using this as a base field.

Multiplication for 2^127-1 is complicated because there is no instruction
that performs 128x128->256 bit products.  So it requires 4 MUL instructions.
Reduction requires about 10 instructions.
Multiplication for 2^61-1 is done with 1 MUL instruction.
Reduction requires about 7 instructions for repeated muliplies.
In an OEF, the 121-bit multiply is overall less complicated, maybe by 30%?


#### Credits

Software by Christopher A. Taylor <mrcatid@gmail.com>.

Please reach out if you need support or would like to collaborate on a project.
