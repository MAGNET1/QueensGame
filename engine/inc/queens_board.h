#ifndef QUEENS_BOARD_H
#define QUEENS_BOARD_H

#include <constants.h>

#define IDX(row, column, size) ((row) * (size) + (column))

/* cell structure: */
/* XEPQ CCCC */
/* C - color identifier */
/* Q - queen present */
/* P - player queen present */
/* E - cell eliminated */
/* X - reserved */
typedef uint8 QueensBoard_Cell_t;
typedef uint8 QueensBoard_Size_t;

constexpr QueensBoard_Cell_t COLOR_NONE           = 0u;
constexpr QueensBoard_Cell_t QUEEN_PRESENT        = (1<<4); /* 0001'0000 (The actual queen provided by QueensBoardgen) */
constexpr QueensBoard_Cell_t PLAYER_QUEEN_PRESENT = (1<<5); /* 0010'0000 (Queen placed by player or QueensSolver) */
constexpr QueensBoard_Cell_t CELL_ELIMINATED      = (1<<6); /* 0100'0000 */

typedef struct
{
    QueensBoard_Cell_t* board;
    QueensBoard_Size_t board_size;
} QueensBoard_Board_t;

bool QueensBoard_Create(QueensBoard_Board_t* empty_board_ptr, const QueensBoard_Size_t size);
void QueensBoard_Free(QueensBoard_Board_t* board);
void QueensBoard_ZeroeBoard(QueensBoard_Board_t* board);
void QueensBoard_PrintBoard(const QueensBoard_Board_t* const board);

QueensBoard_Cell_t QueensBoard_GetColor(const QueensBoard_Cell_t cell);
void QueensBoard_SetColor(QueensBoard_Cell_t* cell, const QueensBoard_Cell_t color);
bool QueensBoard_IsQueenPresent(const QueensBoard_Cell_t cell);
void QueensBoard_SetQueen(QueensBoard_Cell_t* cell, const bool present);
bool QueensBoard_IsCellEliminated(const QueensBoard_Cell_t cell);
void QueensBoard_SetCellEliminated(QueensBoard_Cell_t* cell, const bool eliminated);
bool QueensBoard_IsPlayerQueenPresent(const QueensBoard_Cell_t cell);
void QueensBoard_SetPlayerQueen(QueensBoard_Cell_t* cell, const bool present);
bool QueensBoard_IsCellEmpty(const QueensBoard_Cell_t cell);
bool QueensBoard_IsCellEmptyPlayer(const QueensBoard_Cell_t cell); /* not eliminated on no player queen */

#endif /* QUEENS_BOARD_H */
