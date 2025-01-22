#ifndef QUEENS_PERMUTATIONS_H
#define QUEENS_PERMUTATIONS_H

#include <basic_types.h>
#include <stdbool.h>

constexpr sint8 QUEEN_ROW_NOT_EXISTS = -1;

typedef sint8 QueensPermutations_QueenRowIndex_t;

typedef struct
{
    QueensPermutations_QueenRowIndex_t* boards;
    uint32 boards_count;
    uint8 board_size;
    bool success;
} QueensPermutations_Result_t;

QueensPermutations_Result_t QueensPermutations_Generate(uint8 board_size);
bool QueensPermutations_FreeResult(const QueensPermutations_Result_t result);

#endif /* QUEENS_PERMUTATIONS_H */