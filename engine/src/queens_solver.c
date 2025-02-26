#include <queens_solver.h>
#include <queens_board.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef void (*QueensSolver_Strategy)(QueensBoard_Board_t*);

void QueensSolver_Strategy_InvalidQueen(QueensBoard_Board_t* board);
void QueensSolver_Strategy_InvalidElimination(QueensBoard_Board_t* board);
void QueensSolver_Strategy_EliminateQueenSurrounding(QueensBoard_Board_t* board);
void QueensSolver_Strategy_EliminateLeftoverColors(QueensBoard_Board_t* board);
void QueensSolver_Strategy_LastFreeRowOrColumn(QueensBoard_Board_t* board);
void QueensSolver_Strategy_LastAvailableColorShallBeQueen(QueensBoard_Board_t* board);
void QueensSolver_Strategy_GroupOnlyInSingleRowOrColumn(QueensBoard_Board_t* board);
void QueensSolver_Strategy_SingleColorInRowOrColumn(QueensBoard_Board_t* board);

QueensSolver_Strategy strategies[] =
{
    QueensSolver_Strategy_InvalidQueen,
    QueensSolver_Strategy_InvalidElimination,
    QueensSolver_Strategy_EliminateQueenSurrounding,
    QueensSolver_Strategy_EliminateLeftoverColors,
    QueensSolver_Strategy_LastFreeRowOrColumn,
    QueensSolver_Strategy_LastAvailableColorShallBeQueen,
    QueensSolver_Strategy_GroupOnlyInSingleRowOrColumn,
    QueensSolver_Strategy_SingleColorInRowOrColumn,
};

/* Sanity check - check if queen has been placed by the player, but it actually shouldn't be */
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

/* Sanity check - check if there are cells that have been eliminated, but they contain a queen */
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

/* make sure that surrounding cells of each queen are eliminated (entire row, column and closest diagonals) */
void QueensSolver_Strategy_EliminateQueenSurrounding(QueensBoard_Board_t* board)
{
    for (uint8 row = 0; row < board->board_size; row++)
    {
        for (uint8 column = 0; column < board->board_size; column++)
        {
            QueensBoard_Cell_t cell = board->board[IDX(row, column, board->board_size)];
            if (QueensBoard_IsPlayerQueenPresent(cell))
            {
                for (uint8 i = 0; i < board->board_size; i++)
                {
                    QueensBoard_SetCellEliminated(&board->board[IDX(row, i, board->board_size)], true);
                    QueensBoard_SetCellEliminated(&board->board[IDX(i, column, board->board_size)], true);
                }

                /* eliminate surrounding diagonals */
                sint8 directions[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
                for (uint8 i = 0; i < 4; i++)
                {
                    sint8 new_row = (sint8)row + directions[i][0];
                    sint8 new_column = (sint8)column + directions[i][1];
                    if (new_row >= 0 && new_row < board->board_size && new_column >= 0 && new_column < board->board_size)
                    {
                        QueensBoard_SetCellEliminated(&board->board[IDX(new_row, new_column, board->board_size)], true);
                    }
                }
            }
        }
    }
}

/* if queen of a given color has been placed, all the cells which share the same color, shall be eliminated */
void QueensSolver_Strategy_EliminateLeftoverColors(QueensBoard_Board_t* board)
{
    for (uint8 row = 0; row < board->board_size; row++)
    {
        for (uint8 column = 0; column < board->board_size; column++)
        {
            QueensBoard_Cell_t cell = board->board[IDX(row, column, board->board_size)];
            if (QueensBoard_IsPlayerQueenPresent(cell))
            {
                uint8 color = QueensBoard_GetColor(cell);
                for (uint8 i = 0; i < board->board_size; i++)
                {
                    for (uint8 j = 0; j < board->board_size; j++)
                    {
                        QueensBoard_Cell_t cell = board->board[IDX(i, j, board->board_size)];
                        if (QueensBoard_GetColor(cell) == color)
                        {
                            QueensBoard_SetCellEliminated(&cell, true);
                            board->board[IDX(i, j, board->board_size)] = cell;
                        }
                    }
                }
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
            if (QueensBoard_IsCellEmptyPlayer(cell))
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
            assert(QueensBoard_IsQueenPresent(cell) == true);
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
            QueensBoard_SetPlayerQueen(&board->board[IDX(last_free_row, column, board->board_size)], true);
            return;
        }
    }
}

/* Basic strategy - if a color group doesn't yet have a queen and there's only one left, then it must be a queen */
void QueensSolver_Strategy_LastAvailableColorShallBeQueen(QueensBoard_Board_t* board)
{
    uint8* colors = (uint8*)calloc(board->board_size+1, sizeof(uint8));
    uint8* color_queen_placed = (uint8*)calloc(board->board_size+1, sizeof(uint8));

    if ((colors == NULL) ||
        (color_queen_placed == NULL))
    {
        return;
    }

    for (uint8 row = 0; row < board->board_size; row++)
    {
        for (uint8 column = 0; column < board->board_size; column++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == false)
            {
                uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                colors[color]++;
            }

            if (QueensBoard_IsPlayerQueenPresent(board->board[IDX(row, column, board->board_size)]) == true)
            {
                uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                color_queen_placed[color]++;
            }
        }
    }

    for (uint8 color = 1; color <= board->board_size; color++)
    {
        if ((colors[color] == 1u) &&
            (color_queen_placed[color] == 0u))
        {
            for (uint8 row = 0; row < board->board_size; row++)
            {
                for (uint8 column = 0; column < board->board_size; column++)
                {
                    uint8 cell_color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                    if ((cell_color == color) &&
                        (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true))
                    {
                        QueensBoard_SetPlayerQueen(&board->board[IDX(row, column, board->board_size)], true);
                        free(colors);
                        free(color_queen_placed);
                        return;
                    }
                }
            }
        }
    }

    free(colors);
    free(color_queen_placed);
}

/* If cells from a given color group are left only withing a single row/column, the other color groups can't have a queen placed there. */
/* As a result, all the colors but the investigated ones from from a given row/column can be eliminated */
void QueensSolver_Strategy_GroupOnlyInSingleRowOrColumn(QueensBoard_Board_t* board)
{
    uint8* colors = (uint8*)calloc(board->board_size+1, sizeof(uint8));
    if (colors == NULL)
    {
        return;
    }

    for (uint8 row = 0; row < board->board_size; row++)
    {
        memset(colors, 0, board->board_size+1);

        for (uint8 column = 0; column < board->board_size; column++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
            {
                uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                colors[color]++;
            }
        }

        for (uint8 color = 1; color <= board->board_size; color++)
        {
            /* at least 2 cells of a given color in a row are present */
            /* no need to check for only 1 cell, as it would have been already solved by QueensSolver_Strategy_LastAvailableColorShallBeQueen */
            if (colors[color] > 1u)
            {
                bool color_in_other_row_found = false;

                for (uint8 color_row = 0; color_row < board->board_size; color_row++)
                {
                    if (color_row == row)
                    {
                        continue;
                    }

                    for (uint8 color_column = 0; color_column < board->board_size; color_column++)
                    {
                        if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(color_row, color_column, board->board_size)]) == true)
                        {
                            uint8 cell_color = QueensBoard_GetColor(board->board[IDX(color_row, color_column, board->board_size)]);
                            if (cell_color == color)
                            {
                                color_in_other_row_found = true;
                                break;
                            }
                        }
                    }
                }

                if (color_in_other_row_found == false)
                {
                    for (uint8 color_row = 0; color_row < board->board_size; color_row++)
                    {
                        for (uint8 color_column = 0; color_column < board->board_size; color_column++)
                        {
                            if (color_row == row)
                            {
                                continue;
                            }

                            uint8 cell_color = QueensBoard_GetColor(board->board[IDX(color_row, color_column, board->board_size)]);
                            if (cell_color != color)
                            {
                                QueensBoard_SetCellEliminated(&board->board[IDX(color_row, color_column, board->board_size)], true);
                            }
                        }
                    }

                    free(colors);
                    return;
                }
            }
        }
    }

    for (uint8 column = 0; column < board->board_size; column++)
    {
        memset(colors, 0, board->board_size+1);

        for (uint8 row = 0; row < board->board_size; row++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
            {
                uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                colors[color]++;
            }
        }

        for (uint8 color = 1; color <= board->board_size; color++)
        {
            if (colors[color] > 1u)
            {
                bool color_in_other_column_found = false;

                for (uint8 color_column = 0; color_column < board->board_size; color_column++)
                {
                    if (color_column == column)
                    {
                        continue;
                    }

                    for (uint8 color_row = 0; color_row < board->board_size; color_row++)
                    {
                        if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(color_row, color_column, board->board_size)]) == true)
                        {
                            uint8 cell_color = QueensBoard_GetColor(board->board[IDX(color_row, color_column, board->board_size)]);
                            if (cell_color == color)
                            {
                                color_in_other_column_found = true;
                                break;
                            }
                        }
                    }
                }

                if (color_in_other_column_found == false)
                {
                    for (uint8 color_column = 0; color_column < board->board_size; color_column++)
                    {
                        for (uint8 color_row = 0; color_row < board->board_size; color_row++)
                        {
                            if (color_column == column)
                            {
                                continue;
                            }

                            uint8 cell_color = QueensBoard_GetColor(board->board[IDX(color_row, color_column, board->board_size)]);
                            if (cell_color != color)
                            {
                                QueensBoard_SetCellEliminated(&board->board[IDX(color_row, color_column, board->board_size)], true);
                            }
                        }
                    }

                    free(colors);
                    return;
                }
            }
        }
    }

    free(colors);
    return;
}

/* If in a given column/row there's only one color left, a queen will certainly be placed in one of the cells */
/* This means, that for other columns/rows, there can't be any queen and these cells can be eliminated (only one queen per color is possible) */
void QueensSolver_Strategy_SingleColorInRowOrColumn(QueensBoard_Board_t* board)
{
    uint8* colors = (uint8*)calloc(board->board_size+1, sizeof(uint8));
    if (colors == NULL)
    {
        return;
    }

    for (uint8 row = 0; row < board->board_size; row++)
    {
        memset(colors, 0, board->board_size+1);

        for (uint8 column = 0; column < board->board_size; column++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
            {
                uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                colors[color]++;
            }
        }

        uint8 first_present_color = 0u;
        for (uint8 color = 1; color <= board->board_size; color++)
        {
            if (colors[color] > 0u)
            {
                if (first_present_color == 0u)
                {
                    first_present_color = color;
                }
                else
                {
                    first_present_color = 0u;
                    break;
                }
            }
        }

        if (first_present_color != 0u)
        {
            for (uint8 color_row = 0; color_row < board->board_size; color_row++)
            {
                if (color_row == row)
                {
                    continue;
                }

                for (uint8 color_column = 0; color_column < board->board_size; color_column++)
                {
                    if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(color_row, color_column, board->board_size)]) == true)
                    {
                        uint8 cell_color = QueensBoard_GetColor(board->board[IDX(color_row, color_column, board->board_size)]);
                        if (cell_color == first_present_color)
                        {
                            QueensBoard_SetCellEliminated(&board->board[IDX(color_row, color_column, board->board_size)], true);
                        }
                    }
                }
            }

            free(colors);
            return;
        }
    }

    for (uint8 column = 0; column < board->board_size; column++)
    {
        memset(colors, 0, board->board_size+1);

        for (uint8 row = 0; row < board->board_size; row++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
            {
                uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                colors[color]++;
            }
        }

        uint8 first_present_color = 0u;
        for (uint8 color = 1; color <= board->board_size; color++)
        {
            if (colors[color] > 0u)
            {
                if (first_present_color == 0u)
                {
                    first_present_color = color;
                }
                else
                {
                    first_present_color = 0u;
                    break;
                }
            }
        }

        if (first_present_color != 0u)
        {
            for (uint8 color_column = 0; color_column < board->board_size; color_column++)
            {
                if (color_column == column)
                {
                    continue;
                }

                for (uint8 color_row = 0; color_row < board->board_size; color_row++)
                {
                    if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(color_row, color_column, board->board_size)]) == true)
                    {
                        uint8 cell_color = QueensBoard_GetColor(board->board[IDX(color_row, color_column, board->board_size)]);
                        if (cell_color == first_present_color)
                        {
                            QueensBoard_SetCellEliminated(&board->board[IDX(color_row, color_column, board->board_size)], true);
                        }
                    }
                }
            }

            free(colors);
            return;
        }
    }

    free(colors);
    return;
}
