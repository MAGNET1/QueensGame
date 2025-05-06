#ifndef QUEENS_PERMUTATIONS_H
#define QUEENS_PERMUTATIONS_H

#include <basic_types.h>
#include <stdbool.h>

#include <constants.h>

constexpr sint8 QUEEN_ROW_NOT_EXISTS = -1;

typedef sint8 QueensPermutations_QueenRowIndex_t;
typedef uint8 QueensPermutation_BoardSize_t;

typedef struct
{
    QueensPermutations_QueenRowIndex_t* boards;
    uint32 boards_count;
    QueensPermutation_BoardSize_t board_size;
    bool success;
} QueensPermutations_Result_t;

[[maybe_unused]] QueensPermutations_Result_t QueensPermutations_GetAll(QueensPermutation_BoardSize_t board_size);
[[maybe_unused]] QueensPermutations_Result_t QueensPermutations_GetRandom(const QueensPermutation_BoardSize_t board_size);
bool QueensPermutations_FreeResult(const QueensPermutations_Result_t* result);

#endif /* QUEENS_PERMUTATIONS_H */
