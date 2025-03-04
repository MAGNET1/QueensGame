#include <queens_solver.h>
#include <queens_board.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef void (*QueensSolver_Strategy)(QueensBoard_Board_t*);

typedef enum
{
    QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_NONE = 0,
    QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_DYNAMIC_WINDOW,
    QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_CUSTOM_WINDOW,
    QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_DONE
} QueensSolver_Strategy_NGroups_Step_t;

constexpr uint8 COLOR_PRESENT = 1u;

QueensBoard_Board_t* QueensSolver_CloneBoard(QueensBoard_Board_t* board);
void QueensSolver_CopyBoard(QueensBoard_Board_t* src, QueensBoard_Board_t* dest);
void QueensSolver_PlaceQueen(QueensBoard_Board_t* board, uint8 row, uint8 column);

void QueensSolver_Strategy_InvalidQueen(QueensBoard_Board_t* board);
void QueensSolver_Strategy_InvalidElimination(QueensBoard_Board_t* board);
void QueensSolver_Strategy_EliminateQueenSurrounding(QueensBoard_Board_t* board);
void QueensSolver_Strategy_EliminateLeftoverColors(QueensBoard_Board_t* board);
void QueensSolver_Strategy_LastFreeRowOrColumn(QueensBoard_Board_t* board);
void QueensSolver_Strategy_LastAvailableColorShallBeQueen(QueensBoard_Board_t* board);
void QueensSolver_Strategy_GroupOnlyInSingleRowOrColumn(QueensBoard_Board_t* board);
void QueensSolver_Strategy_SingleColorInRowOrColumn(QueensBoard_Board_t* board);
void QueensSolver_Strategy_QueenPlacementEliminatesAllTheColorsLeft(QueensBoard_Board_t* board);
void QueensSolver_Strategy_QueenPlacementEliminatesEntireRowOrColumn(QueensBoard_Board_t* board);
void QueensSolver_Strategy_NColorGroupsOccupyingNRownsOrColumns(QueensBoard_Board_t* board);
// void QueensSolver_Strategy_QueenPlacementLeadsToInvalidForcingSequence(QueensBoard_Board_t* board);

static inline uint8 QueensSolver_RoundUpDiv(uint8 num, uint8 div);

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
    QueensSolver_Strategy_QueenPlacementEliminatesAllTheColorsLeft
};

QueensBoard_Board_t* QueensSolver_CloneBoard(QueensBoard_Board_t* board)
{
    QueensBoard_Board_t* new_board = (QueensBoard_Board_t*)malloc(sizeof(QueensBoard_Board_t));
    if (new_board == NULL)
    {
        return NULL;
    }
    new_board->board_size = board->board_size;
    new_board->board = (QueensBoard_Cell_t*)malloc(sizeof(QueensBoard_Cell_t)*board->board_size*board->board_size);

    if (new_board->board == NULL)
    {
        free(new_board);
        return NULL;
    }

    memcpy(new_board->board, board->board, sizeof(QueensBoard_Cell_t)*board->board_size*board->board_size);

    return new_board;
}

void QueensSolver_CopyBoard(QueensBoard_Board_t* src, QueensBoard_Board_t* dest)
{
    dest->board_size = src->board_size;
    memcpy(dest->board, src->board, sizeof(QueensBoard_Cell_t)*src->board_size*src->board_size);
}

void QueensSolver_PlaceQueen(QueensBoard_Board_t* board, uint8 row, uint8 column)
{
    QueensBoard_SetPlayerQueen(&board->board[IDX(row, column, board->board_size)], true);

    /* eliminate column, row */
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
        while (new_row >= 0 && new_row < board->board_size && new_column >= 0 && new_column < board->board_size)
        {
            QueensBoard_SetCellEliminated(&board->board[IDX(new_row, new_column, board->board_size)], true);
        }
    }
}

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

/* Check if after placing a queen on a given cell, all the colors from any group gets eliminated. If so, the cell shall be eliminated */
void QueensSolver_Strategy_QueenPlacementEliminatesAllTheColorsLeft(QueensBoard_Board_t* board)
{
    QueensBoard_Board_t board_copy;
    board_copy.board = (QueensBoard_Cell_t*)malloc(sizeof(QueensBoard_Cell_t)*board->board_size*board->board_size);

    uint8* colors = (uint8*)calloc(board->board_size+1, sizeof(uint8));
    uint8* queen_colors = (uint8*)calloc(board->board_size+1, sizeof(uint8));

    if ((colors == NULL) ||
        (queen_colors == NULL) ||
        (board_copy.board == NULL))
    {
        return;
    }

    for (uint8 row = 0; row < board->board_size; row++)
    {
        for (uint8 column = 0; column < board->board_size; column++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
            {
                QueensSolver_CopyBoard(board, &board_copy);
                QueensSolver_PlaceQueen(&board_copy, row, column);

                memset(colors, 0, board->board_size+1);

                for (uint8 i = 0; i < board->board_size; i++)
                {
                    for (uint8 j = 0; j < board->board_size; j++)
                    {
                        uint8 color = QueensBoard_GetColor(board_copy.board[IDX(i, j, board->board_size)]);

                        if (QueensBoard_IsCellEmptyPlayer(board_copy.board[IDX(i, j, board->board_size)]) == true)
                        {
                            colors[color]++;
                        }

                        if (QueensBoard_IsPlayerQueenPresent(board_copy.board[IDX(i, j, board->board_size)]) == true)
                        {
                            queen_colors[color]++;
                        }
                    }
                }

                for (uint8 color = 1; color <= board->board_size; color++)
                {
                    if ((colors[color] == 0u) &&
                        (queen_colors[color] == 0u))
                    {
                        QueensBoard_SetCellEliminated(&board->board[IDX(row, column, board->board_size)], true);

                        free(board_copy.board);
                        free(colors);
                        free(queen_colors);
                        return;
                    }
                }
            }
        }
    }

    free(board_copy.board);
    free(colors);
    free(queen_colors);
    return;
}

/* Check if after placing a queen on a given cell, an entire row/column will be eliminated. If so, the cell shall be eliminated */
void QueensSolver_Strategy_QueenPlacementEliminatesEntireRowOrColumn(QueensBoard_Board_t* board)
{
    QueensBoard_Board_t board_copy;
    board_copy.board = (QueensBoard_Cell_t*)malloc(sizeof(QueensBoard_Cell_t)*board->board_size*board->board_size);

    if (board_copy.board == NULL)
    {
        return;
    }

    for (uint8 row = 0; row < board->board_size; row++)
    {
        for (uint8 column = 0; column < board->board_size; column++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
            {
                QueensSolver_CopyBoard(board, &board_copy);
                QueensSolver_PlaceQueen(&board_copy, row, column);

                uint8 eliminated_count = 0u;
                for (uint8 i = 0; i < board->board_size; i++)
                {
                    if (QueensBoard_IsCellEliminated(board_copy.board[IDX(row, i, board->board_size)]) == true)
                    {
                        eliminated_count++;
                    }
                }

                if (eliminated_count == board->board_size-1)
                {
                    QueensBoard_SetCellEliminated(&board->board[IDX(row, column, board->board_size)], true);
                    continue;
                }

                eliminated_count = 0u;
                for (uint8 i = 0; i < board->board_size; i++)
                {
                    if (QueensBoard_IsCellEliminated(board_copy.board[IDX(i, column, board->board_size)]) == true)
                    {
                        eliminated_count++;
                    }
                }

                if (eliminated_count == board->board_size-1)
                {
                    QueensBoard_SetCellEliminated(&board->board[IDX(row, column, board->board_size)], true);
                    continue;
                }
            }
        }
    }

    free(board_copy.board);
}

/* If N color groups are confined within N rows/columns, N Queens will need to be placed there. Hence, all the colors that do not belong to specified N groups, can be eliminated */
void QueensSolver_Strategy_NColorGroupsOccupyingNRownsOrColumns(QueensBoard_Board_t* board)
{
    constexpr uint8 window_custom_patterns_count = 4u;
    constexpr uint8 window_custom_patterns_len = 4u;
    constexpr uint8 window_custom_patterns[window_custom_patterns_count][window_custom_patterns_len] =
    {
        {1u, 0u, 1u, 0u},
        {1u, 0u, 0u, 1u},
        {1u, 0u, 1u, 1u},
        {1u, 1u, 0u, 1u},
    };

    uint8* colors_in_row                     = (uint8*)calloc(board->board_size*(board->board_size+1), sizeof(uint8));
    uint8* colors_in_column                  = (uint8*)calloc(board->board_size*(board->board_size+1), sizeof(uint8));
    uint8* colors_in_window_row              = (uint8*)calloc(board->board_size+1, sizeof(uint8));
    uint8* colors_in_window_column           = (uint8*)calloc(board->board_size+1, sizeof(uint8));
    uint8* colors_outside_window_row         = (uint8*)calloc(board->board_size+1, sizeof(uint8));
    uint8* colors_outside_window_column      = (uint8*)calloc(board->board_size+1, sizeof(uint8));
    uint8 colors_in_window_row_count         = 0u;
    uint8 colors_in_window_column_count      = 0u;
    uint8 colors_outside_window_row_count    = 0u;
    uint8 colors_outside_window_column_count = 0u;

    uint8* rolling_window = (uint8*)calloc(board->board_size, sizeof(uint8));
    QueensSolver_Strategy_NGroups_Step_t step = QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_NONE;
    uint8 window_custom_pattern_idx = 0u;
    uint8 window_custom_pattern_offset = 0u;


    if ((colors_in_row == NULL) ||
        (colors_in_column == NULL))
    {
        return;
    }

    /* determine which colors are in a given row/column */
    for (uint8 row = 0; row < board->board_size; row++)
    {
        for (uint8 column = 0; column < board->board_size; column++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
            {
                uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                colors_in_row[IDX(row, color, board->board_size)] = COLOR_PRESENT;
                colors_in_column[IDX(column, color, board->board_size)] = COLOR_PRESENT;
            }
        }
    }

    /* rc = row/column */
    uint8 rc_window_begin = 0u;
    uint8 rc_window_end = 0u;
    uint8 dynamic_window_size = 0u;
    uint8 rc_count = 0u;

    while (step != QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_DONE)
    {
        /* 1. handling window pre */
        switch(step)
        {
            case QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_NONE:
                step = QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_DYNAMIC_WINDOW;
                dynamic_window_size = 2u;
                rc_count = dynamic_window_size;
                rc_window_begin = 0u;
                rc_window_end = dynamic_window_size-1;
                /* fall through */

            case QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_DYNAMIC_WINDOW:
                /* build rolling window */
                memset(rolling_window, 0, board->board_size);
                for (uint8 i = rc_window_begin; i <= rc_window_end; i++)
                {
                    rolling_window[i] = 1u;
                }
                break;

            case QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_CUSTOM_WINDOW:
                /* build rolling window */
                memset(rolling_window, 0, board->board_size);
                for (uint8 i = 0; i < window_custom_patterns_count; i++)
                {
                    rolling_window[i+window_custom_pattern_offset] = window_custom_patterns[window_custom_pattern_idx][i];
                }

                rc_count = 0u;
                for (uint8 i = 0; i < board->board_size; i++)
                {
                    if (rolling_window[i] == 1u)
                    {
                        rc_count++;
                    }
                }

                break;

            default:
                assert(false);
                break;

        }

        /* 2. check */
        memset(colors_in_window_row, 0, board->board_size+1);
        memset(colors_in_window_column, 0, board->board_size+1);
        memset(colors_outside_window_row, 0, board->board_size+1);
        memset(colors_outside_window_column, 0, board->board_size+1);
        colors_in_window_row_count = 0u;
        colors_in_window_column_count = 0u;
        colors_outside_window_row_count = 0u;
        colors_outside_window_column_count = 0u;

        /* identify colors present in the window */
        for (uint8 rc = 0; rc < board->board_size; rc++)
        {
            if (rolling_window[rc] == 0u)
            {
                continue;
            }

            for (uint8 color = 1; color <= board->board_size; color++)
            {
                if (colors_in_row[IDX(rc, color, board->board_size)] == COLOR_PRESENT)
                {
                    colors_in_window_row[color] = COLOR_PRESENT;
                    colors_in_window_row_count++;
                }

                if (colors_in_column[IDX(rc, color, board->board_size)] == COLOR_PRESENT)
                {
                    colors_in_window_column[color] = COLOR_PRESENT;
                    colors_in_window_column_count++;
                }
            }
        }

        assert(colors_in_window_row_count >= rc_count);
        assert(colors_in_window_column_count >= rc_count);

        /* if there are N colors in N rows/columns, N queens with investigated colors will have to be placed there */
        /* for that reason, if these N colors appear anywhere outside of window, they can be eliminated */
        if (colors_in_window_column_count == rc_count)
        {
            for (uint8 column = 0; column < board->board_size; column++)
            {
                if (rolling_window[column] == 0u)
                {
                    for (uint8 row = 0; row < board->board_size; row++)
                    {
                        if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
                        {
                            uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                            if (colors_in_window_column[color] == 1u)
                            {
                                QueensBoard_SetCellEliminated(&board->board[IDX(row, column, board->board_size)], true);
                            }
                        }
                    }
                }
            }

            goto free_resources;
        }

        if (colors_in_window_row_count == rc_count)
        {
            for (uint8 row = 0; row < board->board_size; row++)
            {
                if (rolling_window[row] == 0u)
                {
                    for (uint8 column = 0; column < board->board_size; column++)
                    {
                        if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
                        {
                            uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                            if (colors_in_window_row[color] == 1u)
                            {
                                QueensBoard_SetCellEliminated(&board->board[IDX(row, column, board->board_size)], true);
                            }
                        }
                    }
                }
            }

            goto free_resources;
        }

        /* at this point we're certain that there's more than N colors (M from now on) in N rows/columns */
        /* check if N out of M colors are fully contained within N rows/column. If yes, any colors that do not belong to N can be eliminated */
        for (uint8 color = 1; color <= board->board_size; color++)
        {
            if ((colors_in_window_row[color] == 0u) &&
                (colors_in_window_column[color] == 0u))
            {
                continue;
            }

            for (uint8 rc = 0; rc < board->board_size; rc++)
            {
                if (rolling_window[rc] == 0u)
                {
                    if (colors_in_row[IDX(rc, color, board->board_size)] == 1u)
                    {
                        colors_outside_window_row[color] = 1u;
                        colors_outside_window_row_count++;
                    }

                    if (colors_in_column[IDX(rc, color, board->board_size)] == 1u)
                    {
                        colors_outside_window_column[color] = 1u;
                        colors_outside_window_column_count++;
                    }
                }
            }
        }

        if (colors_outside_window_row_count == board->board_size - rc_count)
        {
            for (uint8 row = 0; row < board->board_size; row++)
            {
                if (rolling_window[row] == 1u)
                {
                    for (uint8 column = 0; column < board->board_size; column++)
                    {
                        if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
                        {
                            uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                            if (colors_outside_window_row[color] == 1u)
                            {
                                QueensBoard_SetCellEliminated(&board->board[IDX(row, column, board->board_size)], true);
                            }
                        }
                    }
                }
            }

            goto free_resources;
        }

        if (colors_outside_window_column_count == board->board_size - rc_count)
        {
            for (uint8 column = 0; column < board->board_size; column++)
            {
                if (rolling_window[column] == 1u)
                {
                    for (uint8 row = 0; row < board->board_size; row++)
                    {
                        if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
                        {
                            uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                            if (colors_outside_window_column[color] == 1u)
                            {
                                QueensBoard_SetCellEliminated(&board->board[IDX(row, column, board->board_size)], true);
                            }
                        }
                    }
                }
            }

            goto free_resources;
        }

        /* 3. handling window post */
        switch(step)
        {
            case QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_DYNAMIC_WINDOW:
                rc_window_begin++;
                rc_window_end++;
                if (rc_window_end == board->board_size)
                {
                    dynamic_window_size++;
                    rc_window_begin = 0u;
                    rc_window_end = dynamic_window_size-1;
                    rc_count = dynamic_window_size;

                    if (dynamic_window_size > QueensSolver_RoundUpDiv(dynamic_window_size, 2))
                    {
                        step = QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_CUSTOM_WINDOW;
                    }
                }
                break;

            case QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_CUSTOM_WINDOW:
                window_custom_pattern_offset++;
                if (window_custom_pattern_offset == board->board_size - window_custom_patterns_len)
                {
                    window_custom_pattern_offset = 0u;
                    window_custom_pattern_idx++;
                }

                if (window_custom_pattern_idx == window_custom_patterns_count)
                {
                    step = QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_DONE;
                    goto free_resources;
                }
                break;

            case QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_DONE:
                break;

            default:
                assert(false);
                break;
        }
    }

free_resources:
    free(colors_in_row);
    free(colors_in_window_row);
    free(colors_in_column);
    free(colors_in_window_column);
    free(colors_outside_window_row);
    free(colors_outside_window_column);
    free(rolling_window);
    return;


}

/* example: QueensPermutations_RoundUpDiv(15, 2) == 8 */
static inline uint8 QueensSolver_RoundUpDiv(uint8 num, uint8 div)
{
    return ((num + div - 1u) / div);
}
