#include <queens_board.h>

QueensBoard_Cell_t QueensBoard_GetColor(const QueensBoard_Cell_t cell)
{
    return (cell & 0x0F);
}

void QueensBoard_SetColor(QueensBoard_Cell_t* cell, const QueensBoard_Cell_t color)
{
    *cell = (*cell & 0xF0) | (color & 0x0F);
}

bool QueensBoard_IsQueenPresent(const QueensBoard_Cell_t cell)
{
    return (cell & QUEEN_PRESENT) != 0;
}

void QueensBoard_SetQueen(QueensBoard_Cell_t* cell, const bool present)
{
    if (present)
    {
        *cell |= QUEEN_PRESENT;
    }
    else
    {
        *cell &= ~QUEEN_PRESENT;
    }
}
