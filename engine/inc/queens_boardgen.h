#ifndef QUEENS_BOARDGEN_H
#define QUEENS_BOARDGEN_H

#include <queens_board.h>

typedef enum
{
    QUEENS_BOARDGEN_SUCCESS = 0,
    QUEENS_BOARDGEN_ERROR = 1
} QueensBoardGen_Result_t;

QueensBoardGen_Result_t QueensBoardGen_Generate(QueensBoard_Board_t* board);
void QueensBoardGen_PrintBoard(const QueensBoard_Board_t* const board);
bool QueensBoardGen_ValidateOnlyOneSolution(const QueensBoard_Board_t* board);

#endif /* QUEENS_BOARDGEN_H */
