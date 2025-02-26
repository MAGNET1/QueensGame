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

bool QueensBoard_IsCellEliminated(const QueensBoard_Cell_t cell)
{
    return (cell & CELL_ELIMINATED) != 0;
}

void QueensBoard_SetCellEliminated(QueensBoard_Cell_t* cell, const bool eliminated)
{
    if (eliminated)
    {
        *cell |= CELL_ELIMINATED;
    }
    else
    {
        *cell &= ~CELL_ELIMINATED;
    }
}

bool QueensBoard_IsPlayerQueenPresent(const QueensBoard_Cell_t cell)
{
    return (cell & PLAYER_QUEEN_PRESENT) != 0;
}

void QueensBoard_SetPlayerQueen(QueensBoard_Cell_t* cell, const bool present)
{
    if (present)
    {
        *cell |= PLAYER_QUEEN_PRESENT;
    }
    else
    {
        *cell &= ~PLAYER_QUEEN_PRESENT;
    }
}

bool QueensBoard_IsCellEmpty(const QueensBoard_Cell_t cell)
{
    return (cell & (QUEEN_PRESENT | PLAYER_QUEEN_PRESENT | CELL_ELIMINATED)) == 0;
}

bool QueensBoard_IsCellEmptyPlayer(const QueensBoard_Cell_t cell)
{
    return (cell & (PLAYER_QUEEN_PRESENT | CELL_ELIMINATED)) == 0;
}
