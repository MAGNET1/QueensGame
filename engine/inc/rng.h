#ifndef RNG_H
#define RNG_H

#include <stdio.h>
#include <basic_types.h>
#include <time.h>

void RNG_Seed(uint64 seed);
uint64 RNG_Random_u64();
uint32 RNG_Random_u32();
uint16 RNG_Random_u16();
uint64 RNG_RandomRange_u64(uint64 min, uint64 max); /* max is inclusive */
uint32 RNG_RandomRange_u32(uint32 min, uint32 max); /* max is inclusive */
uint16 RNG_RandomRange_u16(uint16 min, uint16 max); /* max is inclusive */

#endif /* RNG_H */
