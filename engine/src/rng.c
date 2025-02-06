#include <rng.h>

/* PCG Random Number Generator */

/* PCG state */
static uint64 state = 0x853c49e6748fea9bULL;  /* Default seed */
static constexpr uint64 multiplier = 6364136223846793005ULL;
static constexpr uint64 seed_multiplier = 5291759402843659291ULL; /* to make first number more randomized */
static constexpr uint64 increment = 1442695040888963407ULL;

/* Seed the PCG RNG */
void RNG_Seed(uint64 seed)
{
    state = seed*seed_multiplier;
}

uint64 RNG_Random_u64()
{
    uint64 oldstate = state;
    state = oldstate * multiplier + increment;

    uint64 xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint64 rot = oldstate >> 59u;

    return (xorshifted >> rot) | (xorshifted << ((-rot) & 63));
}

uint32 RNG_Random_u32()
{
    uint64 oldstate = state;
    state = oldstate * multiplier + increment;

    // Ensure proper truncation by explicitly casting to uint32
    uint32 xorshifted = (uint32)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32 rot = (uint32)(oldstate >> 59u);

    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

uint16 RNG_Random_u16()
{
    uint64 oldstate = state;
    state = oldstate * multiplier + increment;

    // Ensure proper truncation by explicitly casting to uint16
    uint16 xorshifted = (uint16)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint16 rot = (uint16)(oldstate >> 59u);

    return (uint16)((xorshifted >> rot) | (xorshifted << ((-rot) & 15)));
}

/* max is inclusive */
uint64 RNG_RandomRange_u64(uint64 min, uint64 max)
{
    return (RNG_Random_u64() % (max-min+1)) + min;
}

/* max is inclusive */
uint32 RNG_RandomRange_u32(uint32 min, uint32 max)
{
    return (RNG_Random_u32() % (max-min+1)) + min;
}

/* max is inclusive */
uint16 RNG_RandomRange_u16(uint16 min, uint16 max)
{
    return (uint16)(RNG_Random_u16() % (max-min+1)) + min;
}
