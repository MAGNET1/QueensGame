#include <queens_solver.h>
#include <queens_board.h>

typedef void (*QueensSolver_Strategy)(QueensBoard_Board_t*);

void QueensSolver_Strategy_InvalidElimination(QueensBoard_Board_t* board);
void QueensSolver_Strategy_InvalidQueen(QueensBoard_Board_t* board);
void QueensSolver_Strategy_LastFreeRowOrColumn(QueensBoard_Board_t* board);

QueensSolver_Strategy strategies[] =
{
    QueensSolver_Strategy_InvalidElimination,
    QueensSolver_Strategy_InvalidQueen,
    QueensSolver_Strategy_LastFreeRowOrColumn
};

/* Sanity check strategy - check if there are cells that have been eliminated, but they contain a queen */
void QueensSolver_Strategy_InvalidElimination(QueensBoard_Board_t* board)
{
    for (uint8 row = 0; row < board->board_size; row++)
    {
        for (uint8 column = 0; column < board->board_size; column++)
        {
            QueensBoard_Cell_t cell = board->board[IDX(row, column, board->board_size)];
            if ((QueensBoard_IsQueenPresent(cell)) &&
                (QueensBoard_IsCellEliminated(cell) == true))
            {
                QueensBoard_SetCellEliminated(&cell, false);
                board->board[IDX(row, column, board->board_size)] = cell;
                return;
            }
        }
    }
}

/* Check if queen has been placed by the player, but it actually shouldn't be */
void QueensSolver_Strategy_InvalidQueen(QueensBoard_Board_t* board)
{
    for (uint8 row = 0; row < board->board_size; row++)
    {
        for (uint8 column = 0; column < board->board_size; column++)
        {
            QueensBoard_Cell_t cell = board->board[IDX(row, column, board->board_size)];
            if ((QueensBoard_IsPlayerQueenPresent(cell)) &&
                (QueensBoard_IsQueenPresent(cell) == false))
            {
                QueensBoard_SetPlayerQueen(&cell, false);
                board->board[IDX(row, column, board->board_size)] = cell;
                return;
            }
        }
    }
}

/* Basic strategy - see if there's last square in row/column that hasn't been eliminated. If so, the queen has to be placed there */
void QueensSolver_Strategy_LastFreeRowOrColumn(QueensBoard_Board_t* board)
{
    for (uint8 row = 0; row < board->board_size; row++)
    {
        uint8 eliminated_count = 0u;
        uint8 last_free_column = 0u;
        for (uint8 column = 0; column < board->board_size; column++)
        {
            QueensBoard_Cell_t cell = board->board[IDX(row, column, board->board_size)];
            if (QueensBoard_IsCellEliminated(cell))
            {
                eliminated_count++;
            }
            else
            {
                last_free_column = column;
            }
        }

        if (eliminated_count == board->board_size-1)
        {
            QueensBoard_Cell_t cell = board->board[IDX(row, last_free_column, board->board_size)];
            QueensBoard_SetPlayerQueen(&cell, true);
            board->board[IDX(row, last_free_column, board->board_size)] = cell;
            return;
        }
    }

    for (uint8 column = 0; column < board->board_size; column++)
    {
        uint8 eliminated_count = 0u;
        uint8 last_free_row = 0u;
        for (uint8 row = 0; row < board->board_size; row++)
        {
            QueensBoard_Cell_t cell = board->board[IDX(row, column, board->board_size)];
            if (QueensBoard_IsCellEliminated(cell))
            {
                eliminated_count++;
            }
            else
            {
                last_free_row = row;
            }
        }

        if (eliminated_count == board->board_size-1)
        {
            QueensBoard_Cell_t cell = board->board[IDX(last_free_row, column, board->board_size)];
            QueensBoard_SetPlayerQueen(&cell, true);
            board->board[IDX(last_free_row, column, board->board_size)] = cell;
            return;
        }
    }
}