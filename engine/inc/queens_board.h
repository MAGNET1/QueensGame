#ifndef QUEENS_BOARD_H
#define QUEENS_BOARD_H

#include <constants.h>

#define IDX(row, column, size) ((row) * (size) + (column))

constexpr uint8 CELL_EMPTY = 0u;
constexpr uint8 COLOR_NONE = 0u;
constexpr uint8 QUEEN_PRESENT = (1<<4); /* 0001'0000 */

/* cell structure: */
/* XXXQ CCCC */
/* Q - queen present */
/* C - color identifier */
/* X - reserved */
typedef uint8 QueensBoard_Cell_t;
typedef uint8 QueensBoard_Size_t;

QueensBoard_Cell_t QueensBoard_GetColor(const QueensBoard_Cell_t cell);
void QueensBoard_SetColor(QueensBoard_Cell_t* cell, const QueensBoard_Cell_t color);
bool QueensBoard_IsQueenPresent(const QueensBoard_Cell_t cell);
void QueensBoard_SetQueen(QueensBoard_Cell_t* cell, const bool present);

typedef struct
{
    QueensBoard_Cell_t* board;
    QueensBoard_Size_t board_size;
} QueensBoard_Board_t;

#endif /* QUEENS_BOARD_H */
