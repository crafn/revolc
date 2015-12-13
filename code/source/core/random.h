#ifndef REVOLC_CORE_RANDOM_H
#define REVOLC_CORE_RANDOM_H

// Thanks R.. http://stackoverflow.com/questions/19083566/what-are-the-better-pseudo-random-number-generator-than-the-lcg-for-lottery-sc
static
U32 temper(U32 x)
{
    x ^= x>>11;
    x ^= x<<7 & 0x9D2C5680;
    x ^= x<<15 & 0xEFC60000;
    x ^= x>>18;
    return x;
}

static
U32 random_u32_base(U64 *seed)
{
    *seed = 6364136223846793005ULL * *seed + 1;
    return temper(*seed >> 32);
}

// Not production quality
static
U32 random_u32(U32 min, U32 max, U64 *seed)
{
	ensure(max > min);
	return random_u32_base(seed)%(max - min) + min;
}

// Not production quality
static
S32 random_s32(S32 min, S32 max, U64 *seed)
{
	ensure(max > min);
	return random_u32_base(seed)%(max - min) + min;
}

// Not production quality
static
F64 random_f64(F64 min, F64 max, U64 *seed)
{
	ensure(max > min);
	F64 f = (F64)random_u32_base(seed)/U32_MAX;
	return f*(max - min) + min;
}

// Not production quality
static
F32 random_f32(F32 min, F32 max, U64 *seed)
{
	ensure(max > min);
	F32 f = (F32)random_u32_base(seed)/U32_MAX;
	return f*(max - min) + min;
}

#endif // REVOLC_CORE_RANDOM_H