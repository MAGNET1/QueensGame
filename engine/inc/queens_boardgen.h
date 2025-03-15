#ifndef QUEENS_BOARDGEN_H
#define QUEENS_BOARDGEN_H

#include <queens_board.h>
#include <queens_permutations.h>

typedef enum
{
    QUEENS_BOARDGEN_SUCCESS = 0,
    QUEENS_BOARDGEN_ERROR = 1
} QueensBoardGen_Result_t;

QueensBoardGen_Result_t QueensBoardGen_Generate(QueensBoard_Board_t* board, QueensPermutations_Result_t* permutation);
bool QueensBoardGen_ValidateOnlyOneSolution(const QueensBoard_Board_t* board, const QueensPermutations_Result_t* permutations);

#endif /* QUEENS_BOARDGEN_H */
