#include <queens_solver.h>
#include <queens_board.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <rng.h>

typedef void (*QueensSolver_StrategyFunc_t)(QueensBoard_Board_t*);

typedef enum
{
    QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_NONE = 0,
    QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_DYNAMIC_WINDOW,
    QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_CUSTOM_WINDOW,
    QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_DONE
} QueensSolver_Strategy_NGroups_Step_t;


constexpr uint8 LAST_COLOR_CELL_POSITION_INVALID = 0xFFu;
constexpr uint8 FORCING_SEQUENCE_CANDIDATE_POSITION_INVALID = 0xFFu;
typedef struct
{
    uint8 row;
    uint8 column;
} QueensSolver_LastColorCellPosition_t;

typedef struct
{
    QueensSolver_Strategy_t strategy;
    QueensSolver_StrategyFunc_t strategy_func;
} QueensSolver_StrategyMapping_t;

typedef struct
{
    uint8* colors_in_window_row;
    uint8* colors_in_window_column;
    uint8* colors_outside_window_row;
    uint8* colors_outside_window_column;
    uint8* colors_in_window_and_not_outside_row;
    uint8* colors_in_window_and_not_outside_column;
    uint8* colors_present_in_row;
    uint8* colors_present_in_column;
    uint8 *rolling_window;

    uint8 rc_count;
} QueensSolver_Strategy_NGroups_Alloc_t;

constexpr uint8 COLOR_PRESENT = 1u;

/* TODO update module, so that it uses Arena-based memory instead of allocating every single time new function is called */

static void QueensSolver_CopyBoard(QueensBoard_Board_t* src, QueensBoard_Board_t* dest);
static void QueensSolver_PlaceQueen(QueensBoard_Board_t* board, uint8 row, uint8 column);
static bool QueensSolver_IsBoardValid(QueensBoard_Board_t* board);
static bool QueensSolver_AreBoardsEqual(QueensBoard_Board_t* board1, QueensBoard_Board_t* board2);
static QueensSolver_LastColorCellPosition_t QueensSolver_GetLastColorCellPosition(QueensBoard_Board_t* board);

static void QueensSolver_Strategy_InvalidQueen(QueensBoard_Board_t* board);
static void QueensSolver_Strategy_InvalidElimination(QueensBoard_Board_t* board);
static void QueensSolver_Strategy_EliminateQueenSurrounding(QueensBoard_Board_t* board);
static void QueensSolver_Strategy_EliminateLeftoverColors(QueensBoard_Board_t* board);
static void QueensSolver_Strategy_LastFreeRowOrColumn(QueensBoard_Board_t* board);
static void QueensSolver_Strategy_LastAvailableColorShallBeQueen(QueensBoard_Board_t* board);
static void QueensSolver_Strategy_GroupOnlyInSingleRowOrColumn(QueensBoard_Board_t* board);
static void QueensSolver_Strategy_SingleColorInRowOrColumn(QueensBoard_Board_t* board);
static void QueensSolver_Strategy_QueenPlacementEliminatesAllTheColorsLeft(QueensBoard_Board_t* board);
static void QueensSolver_Strategy_QueenPlacementEliminatesEntireRowOrColumn(QueensBoard_Board_t* board);
static void QueensSolver_Strategy_NColorGroupsOccupyingNRownsOrColumns(QueensBoard_Board_t* board);
static bool QueensSolver_Strategy_NColorGroupsOccupyingNRownsOrColumns_Algorithm(QueensBoard_Board_t* board, QueensSolver_Strategy_NGroups_Alloc_t *ngroups_alloc);
static void QueensSolver_Strategy_QueenPlacementLeadsToInvalidForcingSequence(QueensBoard_Board_t* board);
static bool QueensSolver_Strategy_QueenPlacementLeadsToInvalidForcingSequence_Algorithm(QueensBoard_Board_t* board, uint8 row_idx, uint8 column_idx);

static inline uint8 QueensSolver_RoundUpDiv(uint8 num, uint8 div);

static const QueensSolver_StrategyMapping_t strategy_mapping[] =
{
    { QUEENS_SOLVER_STRATEGY_INVALID_QUEEN,                                     QueensSolver_Strategy_InvalidQueen },
    { QUEENS_SOLVER_STRATEGY_INVALID_ELIMINATION,                               QueensSolver_Strategy_InvalidElimination },
    { QUEENS_SOLVER_STRATEGY_ELIMINATE_QUEEN_SURROUNDING,                       QueensSolver_Strategy_EliminateQueenSurrounding },
    { QUEENS_SOLVER_STRATEGY_ELIMINATE_LEFTOVER_COLORS,                         QueensSolver_Strategy_EliminateLeftoverColors },
    { QUEENS_SOLVER_STRATEGY_LAST_FREE_ROW_OR_COLUMN,                           QueensSolver_Strategy_LastFreeRowOrColumn },
    { QUEENS_SOLVER_STRATEGY_LAST_AVAILABLE_COLOR_SHALL_BE_QUEEN,               QueensSolver_Strategy_LastAvailableColorShallBeQueen },
    { QUEENS_SOLVER_STRATEGY_GROUP_ONLY_IN_SINGLE_ROW_OR_COLUMN,                QueensSolver_Strategy_GroupOnlyInSingleRowOrColumn },
    { QUEENS_SOLVER_STRATEGY_SINGLE_COLOR_IN_ROW_OR_COLUMN,                     QueensSolver_Strategy_SingleColorInRowOrColumn },
    { QUEENS_SOLVER_STRATEGY_QUEEN_PLACEMENT_ELIMINATES_ENTIRE_ROW_OR_COLUMN,   QueensSolver_Strategy_QueenPlacementEliminatesEntireRowOrColumn },
    { QUEENS_SOLVER_STRATEGY_N_COLOR_GROUPS_OCCUPYING_N_ROWS_OR_COLUMNS,        QueensSolver_Strategy_NColorGroupsOccupyingNRownsOrColumns },
    { QUEENS_SOLVER_STRATEGY_QUEEN_PLACEMENT_ELIMINATES_ALL_THE_COLORS_LEFT,    QueensSolver_Strategy_QueenPlacementEliminatesAllTheColorsLeft },
    { QUEENS_SOLVER_STRATEGY_QUEEN_PLACEMENT_LEADS_TO_INVALID_FORCING_SEQUENCE, QueensSolver_Strategy_QueenPlacementLeadsToInvalidForcingSequence }
};

QueensSolver_Strategy_t QueensSolver_IncrementalSolve(QueensBoard_Board_t* board)
{
    if (QueensSolver_IsBoardSolved(board) == true)
    {
        return QUEENS_SOLVER_SOLVED;
    }

    QueensBoard_Board_t board_copy;
    bool status = QueensBoard_Create(&board_copy, board->board_size);
    QueensSolver_CopyBoard(board, &board_copy);

    if (status == false)
    {
        return QUEENS_SOLVER_FAILED;
    }

    for (uint8 i = QUEENS_SOLVER_STRATEGY_FIRST; i < QUEENS_SOLVER_STRATEGY_LAST; i++)
    {
        strategy_mapping[i].strategy_func(board);
        /* if no change, nothing was solved at a particular step */
        if (QueensSolver_AreBoardsEqual(board, &board_copy) == false)
        {
            QueensBoard_Free(&board_copy);
            return strategy_mapping[i].strategy;
        }
    }

    return QUEENS_SOLVER_FAILED;
}

static bool QueensSolver_AreBoardsEqual(QueensBoard_Board_t* board1, QueensBoard_Board_t* board2)
{
    if (board1->board_size != board2->board_size)
    {
        return false;
    }

    for (uint8 i = 0; i < board1->board_size; i++)
    {
        for (uint8 j = 0; j < board1->board_size; j++)
        {
            if (board1->board[IDX(i, j, board1->board_size)] != board2->board[IDX(i, j, board2->board_size)])
            {
                return false;
            }
        }
    }

    return true;
}

static void QueensSolver_CopyBoard(QueensBoard_Board_t* src, QueensBoard_Board_t* dest)
{
    dest->board_size = src->board_size;
    memcpy(dest->board, src->board, sizeof(QueensBoard_Cell_t)*src->board_size*src->board_size);
}

bool QueensSolver_IsBoardSolved(QueensBoard_Board_t* board)
{
    /* check if one queen per row and column */
    uint8* rows = (uint8*)calloc(board->board_size, sizeof(uint8));
    uint8* columns = (uint8*)calloc(board->board_size, sizeof(uint8));

    if ((rows == NULL) ||
        (columns == NULL))
    {
        return false;
    }

    for (uint8 row_idx = 0; row_idx < board->board_size; row_idx++)
    {
        for (uint8 column_idx = 0; column_idx < board->board_size; column_idx++)
        {
            if (QueensBoard_IsPlayerQueenPresent(board->board[IDX(row_idx, column_idx, board->board_size)]) == true)
            {
                rows[row_idx]++;
                columns[column_idx]++;
            }
        }
    }

    for (uint8 row_column_idx = 0; row_column_idx < board->board_size; row_column_idx++)
    {
        if ((rows[row_column_idx] != 1) ||
            (columns[row_column_idx] != 1))
        {
            free(rows);
            free(columns);
            return false;
        }
    }

    /* no diagonal adjacency */
    for (uint8 row_idx = 0; row_idx < board->board_size; row_idx++)
    {
        for (uint8 column_idx = 0; column_idx < board->board_size; column_idx++)
        {
            if (QueensBoard_IsPlayerQueenPresent(board->board[IDX(row_idx, column_idx, board->board_size)]) == true)
            {
                sint8 directions[4][2] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
                for (uint8 direction_idx = 0; direction_idx < 4; direction_idx++)
                {
                    sint8 new_row = (sint8)row_idx + directions[direction_idx][0];
                    sint8 new_column = (sint8)column_idx + directions[direction_idx][1];
                    if (new_row >= 0 && new_row < board->board_size && new_column >= 0 && new_column < board->board_size)
                    {
                        if (QueensBoard_IsPlayerQueenPresent(board->board[IDX(new_row, new_column, board->board_size)]) == true)
                        {
                            free(rows);
                            free(columns);
                            return false;
                        }
                    }
                }
            }
        }
    }

    /* each queens shall be of unique color */
    uint8* colors = (uint8*)calloc(board->board_size+1, sizeof(uint8));
    if (colors == NULL)
    {
        free(rows);
        free(columns);
        return false;
    }

    for (uint8 i = 0; i < board->board_size; i++)
    {
        for (uint8 j = 0; j < board->board_size; j++)
        {
            if (QueensBoard_IsPlayerQueenPresent(board->board[IDX(i, j, board->board_size)]) == true)
            {
                uint8 color = QueensBoard_GetColor(board->board[IDX(i, j, board->board_size)]);
                colors[color]++;
            }
        }
    }

    for (uint8 i = 1; i <= board->board_size; i++)
    {
        if (colors[i] != 1)
        {
            free(rows);
            free(columns);
            free(colors);
            return false;
        }
    }

    free(rows);
    free(columns);
    free(colors);
    return true;
}

static void QueensSolver_PlaceQueen(QueensBoard_Board_t* board, uint8 row, uint8 column)
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
        if (new_row >= 0 && new_row < board->board_size && new_column >= 0 && new_column < board->board_size)
        {
            QueensBoard_SetCellEliminated(&board->board[IDX(new_row, new_column, board->board_size)], true);
        }
    }
}

/* Sanity check - check if queen has been placed by the player, but it actually shouldn't be */
static void QueensSolver_Strategy_InvalidQueen(QueensBoard_Board_t* board)
{
    /* check if there are any non-player queens. If not, skip the strategy */
    bool queen_present = false;
    for (uint8 row = 0; row < board->board_size; row++)
    {
        for (uint8 column = 0; column < board->board_size; column++)
        {
            if (QueensBoard_IsQueenPresent(board->board[IDX(row, column, board->board_size)]) == true)
            {
                queen_present = true;
                break;
            }
        }
    }

    if (queen_present == false)
    {
        return;
    }

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
static void QueensSolver_Strategy_InvalidElimination(QueensBoard_Board_t* board)
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
static void QueensSolver_Strategy_EliminateQueenSurrounding(QueensBoard_Board_t* board)
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
static void QueensSolver_Strategy_EliminateLeftoverColors(QueensBoard_Board_t* board)
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
static void QueensSolver_Strategy_LastFreeRowOrColumn(QueensBoard_Board_t* board)
{
    for (uint8 row = 0; row < board->board_size; row++)
    {
        uint8 empty_count = 0u;
        uint8 last_free_column = 0u;
        for (uint8 column = 0; column < board->board_size; column++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]))
            {
                empty_count++;
                last_free_column = column;
            }
        }

        if (empty_count == 1u)
        {
            QueensBoard_SetPlayerQueen(&board->board[IDX(row, last_free_column, board->board_size)], true);
            return;
        }
    }

    for (uint8 column = 0; column < board->board_size; column++)
    {
        uint8 empty_count = 0u;
        uint8 last_free_row = 0u;
        for (uint8 row = 0; row < board->board_size; row++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]))
            {
                empty_count++;
                last_free_row = row;
            }
        }

        if (empty_count == 1u)
        {
            QueensBoard_SetPlayerQueen(&board->board[IDX(last_free_row, column, board->board_size)], true);
            return;
        }
    }
}

/* Basic strategy - if a color group doesn't yet have a queen and there's only one left, then it must be a queen */
static void QueensSolver_Strategy_LastAvailableColorShallBeQueen(QueensBoard_Board_t* board)
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
            volatile uint8 dbg_idx = IDX(row, column, board->board_size);

            if (QueensBoard_IsCellEmptyPlayer(board->board[dbg_idx]))
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
static void QueensSolver_Strategy_GroupOnlyInSingleRowOrColumn(QueensBoard_Board_t* board)
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
            if (colors[color] <= 1u)
            {
                continue;
            }

            /* at least 2 colors are present in a row */
            uint8 colors_in_row_count = 0u;
            for (uint8 color_check = 1; color_check <= board->board_size; color_check++)
            {
                if (colors[color_check] > 0u)
                {
                    colors_in_row_count++;
                }
            }
            if (colors_in_row_count <= 1u)
            {
                continue;
            }

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
                            goto color_check;
                        }
                    }
                }
            }

        color_check:
            if (color_in_other_row_found == false)
            {
                for (uint8 color_column = 0; color_column < board->board_size; color_column++)
                {
                    uint8 cell_color = QueensBoard_GetColor(board->board[IDX(row, color_column, board->board_size)]);
                    if (cell_color != color)
                    {
                        QueensBoard_SetCellEliminated(&board->board[IDX(row, color_column, board->board_size)], true);
                    }
                }

                free(colors);
                return;
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
            /* at least 2 cells of a given color in a row are present */
            /* no need to check for only 1 cell, as it would have been already solved by QueensSolver_Strategy_LastAvailableColorShallBeQueen */
            if (colors[color] <= 1u)
            {
                continue;
            }

            /* at least 2 colors are present in column */
            uint8 colors_in_column_count = 0u;
            for (uint8 color_check = 1; color_check <= board->board_size; color_check++)
            {
                if (colors[color_check] > 0u)
                {
                    colors_in_column_count++;
                }
            }
            if (colors_in_column_count <= 1u)
            {
                continue;
            }

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
                            goto row_check;
                        }
                    }
                }
            }

        row_check:
            if (color_in_other_column_found == false)
            {
                for (uint8 color_row = 0; color_row < board->board_size; color_row++)
                {
                    uint8 cell_color = QueensBoard_GetColor(board->board[IDX(color_row, column, board->board_size)]);
                    if (cell_color != color)
                    {
                        QueensBoard_SetCellEliminated(&board->board[IDX(color_row, column, board->board_size)], true);
                    }
                }

                free(colors);
                return;
            }
        }
    }

    free(colors);
    return;
}

/* If in a given column/row there's only one color left, a queen will certainly be placed in one of the cells */
/* This means, that for other columns/rows, there can't be any queen and these cells can be eliminated (only one queen per color is possible) */
static void QueensSolver_Strategy_SingleColorInRowOrColumn(QueensBoard_Board_t* board)
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
static void QueensSolver_Strategy_QueenPlacementEliminatesAllTheColorsLeft(QueensBoard_Board_t* board)
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
                memset(queen_colors, 0, board->board_size+1);

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
static void QueensSolver_Strategy_QueenPlacementEliminatesEntireRowOrColumn(QueensBoard_Board_t* board)
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
static void QueensSolver_Strategy_NColorGroupsOccupyingNRownsOrColumns(QueensBoard_Board_t* board)
{
    /* allocate memory for the helper structures */
    size_t colors_in_outside_window_size     = (board->board_size+1)                    * sizeof(uint8);
    size_t colors_present_in_row_window_size =  board->board_size*(board->board_size+1) * sizeof(uint8);
    size_t rolling_window_size               =  board->board_size                       * sizeof(uint8);
    size_t arena_size                        = ((colors_in_outside_window_size     * 6u) +
                                                (colors_present_in_row_window_size * 2u) +
                                                (rolling_window_size               * 1u));

    uint8* arena = (uint8*)calloc(arena_size, sizeof(uint8));
    if (arena == NULL)
    {
        return;
    }

    QueensSolver_Strategy_NGroups_Alloc_t ngroups_alloc;
    ngroups_alloc.colors_in_window_row                    = arena;
    ngroups_alloc.colors_in_window_column                 = ngroups_alloc.colors_in_window_row + colors_in_outside_window_size;
    ngroups_alloc.colors_outside_window_row               = ngroups_alloc.colors_in_window_column + colors_in_outside_window_size;
    ngroups_alloc.colors_outside_window_column            = ngroups_alloc.colors_outside_window_row + colors_in_outside_window_size;
    ngroups_alloc.colors_in_window_and_not_outside_row    = ngroups_alloc.colors_outside_window_column + colors_in_outside_window_size;
    ngroups_alloc.colors_in_window_and_not_outside_column = ngroups_alloc.colors_in_window_and_not_outside_row + colors_in_outside_window_size;
    ngroups_alloc.colors_present_in_row                   = ngroups_alloc.colors_in_window_and_not_outside_column + colors_in_outside_window_size;
    ngroups_alloc.colors_present_in_column                = ngroups_alloc.colors_present_in_row + colors_present_in_row_window_size;
    ngroups_alloc.rolling_window                          = ngroups_alloc.colors_present_in_column + colors_present_in_row_window_size;

    assert((ngroups_alloc.rolling_window + rolling_window_size) == (arena + arena_size));

    constexpr uint8 window_custom_patterns_count = 5u;
    constexpr uint8 window_custom_patterns_len = 4u;
    constexpr uint8 window_custom_patterns[window_custom_patterns_count][window_custom_patterns_len] =
    {
        {1u, 0u, 1u, 0u},
        {0u, 1u, 0u, 1u},
        {1u, 0u, 0u, 1u},
        {1u, 0u, 1u, 1u},
        {1u, 1u, 0u, 1u},
    };

    QueensSolver_Strategy_NGroups_Step_t step = QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_NONE;
    uint8 window_custom_pattern_idx = 0u;
    uint8 window_custom_pattern_offset = 0u;


    /* helper structure: determine which colors are in a given row/column */
    for (uint8 row = 0; row < board->board_size; row++)
    {
        for (uint8 column = 0; column < board->board_size; column++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
            {
                uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                ngroups_alloc.colors_present_in_row[IDX(row, color, board->board_size)] = COLOR_PRESENT;
                ngroups_alloc.colors_present_in_column[IDX(column, color, board->board_size)] = COLOR_PRESENT;
            }
        }
    }

    /* rc = row/column */
    uint8 rc_window_begin = 0u;
    uint8 rc_window_end = 0u;
    uint8 dynamic_window_size = 0u;

    while (step != QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_DONE)
    {
        /* 1. building rolling window */
        switch(step)
        {
            case QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_NONE:
                step = QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_DYNAMIC_WINDOW;
                dynamic_window_size = 2u;
                ngroups_alloc.rc_count = dynamic_window_size;
                rc_window_begin = 0u;
                rc_window_end = dynamic_window_size-1;
                /* fall through */

            case QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_DYNAMIC_WINDOW:
                /* build rolling window */
                memset(ngroups_alloc.rolling_window, 0, board->board_size);
                for (uint8 i = rc_window_begin; i <= rc_window_end; i++)
                {
                    ngroups_alloc.rolling_window[i] = 1u;
                }
                break;

            case QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_CUSTOM_WINDOW:
                /* build rolling window */
                memset(ngroups_alloc.rolling_window, 0, board->board_size);
                for (uint8 i = 0; i < window_custom_patterns_len; i++)
                {
                    ngroups_alloc.rolling_window[i+window_custom_pattern_offset] = window_custom_patterns[window_custom_pattern_idx][i];
                }

                ngroups_alloc.rc_count = 0u;
                for (uint8 i = 0; i < board->board_size; i++)
                {
                    if (ngroups_alloc.rolling_window[i] == 1u)
                    {
                        ngroups_alloc.rc_count++;
                    }
                }

                break;

            default:
                assert(false);
                break;

        }

        /* 2. check if N colors are present in N rows/columns */
        bool success = QueensSolver_Strategy_NColorGroupsOccupyingNRownsOrColumns_Algorithm(board, &ngroups_alloc);
        if (success == true)
        {
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
                    ngroups_alloc.rc_count = dynamic_window_size;

                    if (dynamic_window_size > QueensSolver_RoundUpDiv(board->board_size, 2))
                    {
                        step = QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_CUSTOM_WINDOW;
                    }
                }
                break;

            case QUEENS_SOLVER_STRATEGY_N_GROUPS_STEP_CUSTOM_WINDOW:
                window_custom_pattern_offset++;
                if (window_custom_pattern_offset == board->board_size - window_custom_patterns_len + 1)
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
    free(arena);
    return;
}

static bool QueensSolver_Strategy_NColorGroupsOccupyingNRownsOrColumns_Algorithm(QueensBoard_Board_t* board, QueensSolver_Strategy_NGroups_Alloc_t *ngroups_alloc)
{
    /* check if queen is present in a window, column- and row-wise */
    bool queen_in_window_row_found = false;
    bool queen_in_window_column_found = false;
    for (uint8 rc = 0; rc < board->board_size; rc++)
    {
        if (ngroups_alloc->rolling_window[rc] == 1u)
        {
            for (uint8 cr = 0; cr < board->board_size; cr++)
            {
                if (QueensBoard_IsPlayerQueenPresent(board->board[IDX(rc, cr, board->board_size)]) == true)
                {
                    queen_in_window_row_found = true;
                }

                if (QueensBoard_IsPlayerQueenPresent(board->board[IDX(cr, rc, board->board_size)]) == true)
                {
                    queen_in_window_column_found = true;
                }
            }
        }
    }

    uint8 colors_in_window_row_count                    = 0u;
    uint8 colors_in_window_column_count                 = 0u;
    uint8 colors_outside_window_row_count               = 0u;
    uint8 colors_outside_window_column_count            = 0u;
    uint8 colors_in_window_and_not_outside_row_count    = 0u;
    uint8 colors_in_window_and_not_outside_column_count = 0u;

    memset(ngroups_alloc->colors_in_window_row, 0, board->board_size+1);
    memset(ngroups_alloc->colors_in_window_column, 0, board->board_size+1);
    memset(ngroups_alloc->colors_outside_window_row, 0, board->board_size+1);
    memset(ngroups_alloc->colors_outside_window_column, 0, board->board_size+1);
    memset(ngroups_alloc->colors_in_window_and_not_outside_row, 0, board->board_size+1);
    memset(ngroups_alloc->colors_in_window_and_not_outside_column, 0, board->board_size+1);

    /* identify colors present in the window */
    for (uint8 rc = 0; rc < board->board_size; rc++)
    {
        if (ngroups_alloc->rolling_window[rc] == 0u)
        {
            continue;
        }

        /* identify which colors are present in a window */
        for (uint8 color = 1; color <= board->board_size; color++)
        {
            if ((ngroups_alloc->colors_present_in_row[IDX(rc, color, board->board_size)] == COLOR_PRESENT) &&
                (ngroups_alloc->colors_in_window_row[color] != COLOR_PRESENT))
            {
                ngroups_alloc->colors_in_window_row[color] = COLOR_PRESENT;
                colors_in_window_row_count++;
            }

            if ((ngroups_alloc->colors_present_in_column[IDX(rc, color, board->board_size)] == COLOR_PRESENT) &&
                (ngroups_alloc->colors_in_window_column[color] != COLOR_PRESENT))
            {
                ngroups_alloc->colors_in_window_column[color] = COLOR_PRESENT;
                colors_in_window_column_count++;
            }
        }
    }

    /* if there are N colors in N rows/columns, N queens with investigated colors will have to be placed there */
    /* for that reason, if these N colors appear anywhere outside of window, they can be eliminated */
    if (queen_in_window_column_found == false)
    {
        if (colors_in_window_column_count == ngroups_alloc->rc_count)
        {
            bool eliminated = false;
            for (uint8 column = 0; column < board->board_size; column++)
            {
                if (ngroups_alloc->rolling_window[column] == 0u)
                {
                    for (uint8 row = 0; row < board->board_size; row++)
                    {
                        if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
                        {
                            uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                            if (ngroups_alloc->colors_in_window_column[color] == 1u)
                            {
                                QueensBoard_SetCellEliminated(&board->board[IDX(row, column, board->board_size)], true);
                                eliminated = true;
                            }
                        }
                    }
                }
            }

            if (eliminated == true)
            {
                return true;
            }
        }
    }

    /* same for rows */
    if (queen_in_window_row_found == false)
    {
        if (colors_in_window_row_count == ngroups_alloc->rc_count)
        {
            bool eliminated = false;
            for (uint8 row = 0; row < board->board_size; row++)
            {
                if (ngroups_alloc->rolling_window[row] == 0u)
                {
                    for (uint8 column = 0; column < board->board_size; column++)
                    {
                        if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
                        {
                            uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                            if (ngroups_alloc->colors_in_window_row[color] == 1u)
                            {
                                QueensBoard_SetCellEliminated(&board->board[IDX(row, column, board->board_size)], true);
                                eliminated = true;
                            }
                        }
                    }
                }
            }

            if (eliminated == true)
            {
                return true;
            }
        }
    }

    /* at this point we're certain that there's more than N colors (M from now on) in N rows/columns */
    /* check if N out of M colors are fully contained within N rows/column. If yes, any colors that do not belong to N within the window can be eliminated */

    /* helper structure: determine which colors are present outside of window */
    for (uint8 color = 1; color <= board->board_size; color++)
    {
        for (uint8 rc = 0; rc < board->board_size; rc++)
        {
            if (ngroups_alloc->rolling_window[rc] == 0u)
            {
                if ((ngroups_alloc->colors_present_in_row[IDX(rc, color, board->board_size)] == COLOR_PRESENT) &&
                    (ngroups_alloc->colors_outside_window_row[color] != COLOR_PRESENT))
                {
                    ngroups_alloc->colors_outside_window_row[color] = COLOR_PRESENT;
                    colors_outside_window_row_count++;
                }

                if ((ngroups_alloc->colors_present_in_column[IDX(rc, color, board->board_size)] == COLOR_PRESENT) &&
                    (ngroups_alloc->colors_outside_window_column[color] != COLOR_PRESENT))
                {
                    ngroups_alloc->colors_outside_window_column[color] = COLOR_PRESENT;
                    colors_outside_window_column_count++;
                }
            }
        }
    }

    /* helper structure: determine which colors are present ONLY within the window */
    for (uint8 color = 1; color <= board->board_size; color++)
    {
        if ((ngroups_alloc->colors_in_window_row[color] == 0u) &&
            (ngroups_alloc->colors_in_window_column[color] == 0u))
        {
            continue;
        }

        for (uint8 rc = 0; rc < board->board_size; rc++)
        {
            if (ngroups_alloc->rolling_window[rc] == 1u)
            {
                if ((ngroups_alloc->colors_present_in_row[IDX(rc, color, board->board_size)] == COLOR_PRESENT) &&
                    (ngroups_alloc->colors_outside_window_row[color] != COLOR_PRESENT) &&
                    (ngroups_alloc->colors_in_window_and_not_outside_row[color] != COLOR_PRESENT))
                {
                    ngroups_alloc->colors_in_window_and_not_outside_row[color] = COLOR_PRESENT;
                    colors_in_window_and_not_outside_row_count++;
                }

                if ((ngroups_alloc->colors_present_in_column[IDX(rc, color, board->board_size)] == COLOR_PRESENT) &&
                    (ngroups_alloc->colors_outside_window_column[color] != COLOR_PRESENT) &&
                    (ngroups_alloc->colors_in_window_and_not_outside_column[color] != COLOR_PRESENT))
                {
                    ngroups_alloc->colors_in_window_and_not_outside_column[color] = COLOR_PRESENT;
                    colors_in_window_and_not_outside_column_count++;
                }
            }
        }
    }

    /* Check if there are N colors that exist ONLY within the window. If yes, get rid of other colors in the window */
    if (queen_in_window_row_found == false)
    {
        if ((colors_in_window_and_not_outside_row_count == ngroups_alloc->rc_count) &&
            (colors_outside_window_row_count > 0u))
        {
            bool eliminated = false;
            for (uint8 row = 0; row < board->board_size; row++)
            {
                if (ngroups_alloc->rolling_window[row] == 1u)
                {
                    for (uint8 column = 0; column < board->board_size; column++)
                    {
                        if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
                        {
                            uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                            if (ngroups_alloc->colors_in_window_and_not_outside_row[color] == 0u)
                            {
                                QueensBoard_SetCellEliminated(&board->board[IDX(row, column, board->board_size)], true);
                                eliminated = true;
                            }
                        }
                    }
                }
            }

            if (eliminated == true)
            {
                return true;
            }
        }
    }

    /* same for columns */
    if (queen_in_window_column_found == false)
    {
        if ((colors_in_window_and_not_outside_column_count == ngroups_alloc->rc_count) &&
            (colors_outside_window_column_count > 0u))
        {
            bool eliminated = false;
            for (uint8 column = 0; column < board->board_size; column++)
            {
                if (ngroups_alloc->rolling_window[column] == 1u)
                {
                    for (uint8 row = 0; row < board->board_size; row++)
                    {
                        if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
                        {
                            uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                            if (ngroups_alloc->colors_in_window_and_not_outside_column[color] == 0u)
                            {
                                QueensBoard_SetCellEliminated(&board->board[IDX(row, column, board->board_size)], true);
                                eliminated = true;
                            }
                        }
                    }
                }
            }

            if (eliminated == true)
            {
                return true;
            }
        }
    }

    return false;
}

/* Removes cell triggering a chain of forced moves leading to invalid state */
/* Picks cell whose removal enables a different strategy in the fewest steps */
static void QueensSolver_Strategy_QueenPlacementLeadsToInvalidForcingSequence(QueensBoard_Board_t* board)
{
    QueensBoard_Board_t board_copy_current;
    QueensBoard_Board_t board_copy_new;
    board_copy_current.board = (QueensBoard_Cell_t*)malloc(sizeof(QueensBoard_Cell_t)*board->board_size*board->board_size);
    board_copy_new.board = (QueensBoard_Cell_t*)malloc(sizeof(QueensBoard_Cell_t)*board->board_size*board->board_size);
    if ((board_copy_current.board == NULL) ||
        (board_copy_new.board == NULL))
    {
        return;
    }

    /* details of candidate for elimination. The goal is to find cell with as little Forcing sequence calls as possible, preferably 1 */
    uint8 candidate_row = FORCING_SEQUENCE_CANDIDATE_POSITION_INVALID;
    uint8 candidate_column = FORCING_SEQUENCE_CANDIDATE_POSITION_INVALID;
    uint8 candidate_consecutive_forcing_sequence_call_count = 0u;

    for (uint8 row_idx = 0u; row_idx < board->board_size; row_idx++)
    {
        for (uint8 column_idx = 0u; column_idx < board->board_size; column_idx++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row_idx, column_idx, board->board_size)]) == true)
            {
                QueensSolver_CopyBoard(board, &board_copy_current);
                uint8 consecutive_forcing_sequence_call_count = 0u;
                while (true)
                {
                    QueensSolver_CopyBoard(&board_copy_current, &board_copy_new);

                    bool sequence_found = false;
                    if (consecutive_forcing_sequence_call_count == 0u)
                    {
                        /* first call, use candidated coordinates */
                        sequence_found = QueensSolver_Strategy_QueenPlacementLeadsToInvalidForcingSequence_Algorithm(&board_copy_new, row_idx, column_idx);
                    }
                    else
                    {
                        /* consecutive call, let algorithm find next candidate */
                        sequence_found = QueensSolver_Strategy_QueenPlacementLeadsToInvalidForcingSequence_Algorithm(&board_copy_new, FORCING_SEQUENCE_CANDIDATE_POSITION_INVALID, FORCING_SEQUENCE_CANDIDATE_POSITION_INVALID);
                    }
                    if (sequence_found == true)
                    {
                        consecutive_forcing_sequence_call_count++;
                        QueensBoard_SetCellEliminated(&board_copy_current.board[IDX(row_idx, column_idx, board->board_size)], true);
                        /* updated board will be used for next iteration */
                    }
                    else
                    {
                        /* best case scenario, use this candidate and abort */
                        if (consecutive_forcing_sequence_call_count == 1u)
                        {
                            QueensBoard_SetCellEliminated(&board->board[IDX(row_idx, column_idx, board->board_size)], true);
                            goto free_resources;
                        }

                        /* check if the candidate is better than the previous one */
                        if ((consecutive_forcing_sequence_call_count > 1u) &&
                            (consecutive_forcing_sequence_call_count < candidate_consecutive_forcing_sequence_call_count))
                        {
                            candidate_row = row_idx;
                            candidate_column = column_idx;
                            candidate_consecutive_forcing_sequence_call_count = consecutive_forcing_sequence_call_count;
                        }
                        break;
                    }
                }
            }
        }
    }

    /* check if there's a candidate and eliminate it */
    if ((candidate_row != FORCING_SEQUENCE_CANDIDATE_POSITION_INVALID) &&
        (candidate_column != FORCING_SEQUENCE_CANDIDATE_POSITION_INVALID))
    {
        QueensBoard_SetCellEliminated(&board->board[IDX(candidate_row, candidate_column, board->board_size)], true);
    }

free_resources:
    free(board_copy_current.board);
    free(board_copy_new.board);
}

static bool QueensSolver_Strategy_QueenPlacementLeadsToInvalidForcingSequence_Algorithm(QueensBoard_Board_t* board, uint8 row_idx, uint8 column_idx)
{
    uint8 dest_row = row_idx;
    uint8 dest_column = column_idx;

    if ((dest_row == FORCING_SEQUENCE_CANDIDATE_POSITION_INVALID) ||
        (dest_column == FORCING_SEQUENCE_CANDIDATE_POSITION_INVALID))
    {
        /* no coordinates given, try to find remaining last cell of a color group */
        QueensSolver_LastColorCellPosition_t last_color_cell = QueensSolver_GetLastColorCellPosition(board);
        if ((last_color_cell.row == LAST_COLOR_CELL_POSITION_INVALID) ||
            (last_color_cell.column == LAST_COLOR_CELL_POSITION_INVALID))
        {
            return false;
        }
        else
        {
            dest_row = last_color_cell.row;
            dest_column = last_color_cell.column;
        }
    }

    while(true)
    {
        QueensSolver_PlaceQueen(board, dest_row, dest_column);
        QueensSolver_Strategy_EliminateQueenSurrounding(board);
        QueensSolver_Strategy_EliminateLeftoverColors(board);

        if (QueensSolver_IsBoardValid(board) == false)
        {
            return true;
        }
        else
        {
            QueensSolver_LastColorCellPosition_t last_color_cell = QueensSolver_GetLastColorCellPosition(board);
            if ((last_color_cell.row == LAST_COLOR_CELL_POSITION_INVALID) ||
                (last_color_cell.column == LAST_COLOR_CELL_POSITION_INVALID))
            {
                return false;
            }
            else
            {
                dest_row = last_color_cell.row;
                dest_column = last_color_cell.column;
            }
        }
    }

    return false;
}

/* Look for any color that has one cell left and retrieve its position */
static QueensSolver_LastColorCellPosition_t QueensSolver_GetLastColorCellPosition(QueensBoard_Board_t* board)
{
    QueensSolver_LastColorCellPosition_t last_color_cell;
    last_color_cell.row = LAST_COLOR_CELL_POSITION_INVALID;
    last_color_cell.column = LAST_COLOR_CELL_POSITION_INVALID;


    uint8* colors = (uint8*)calloc(board->board_size+1, sizeof(uint8));
    if (colors == NULL)
    {
        return last_color_cell;
    }

    for (uint8 row = 0; row < board->board_size; row++)
    {
        for (uint8 column = 0; column < board->board_size; column++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
            {
                uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                colors[color]++;
            }
        }
    }

    for (uint8 color = 1; color <= board->board_size; color++)
    {
        if (colors[color] == 1u)
        {
            for (uint8 row = 0; row < board->board_size; row++)
            {
                for (uint8 column = 0; column < board->board_size; column++)
                {
                    if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true)
                    {
                        uint8 cell_color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);
                        if (cell_color == color)
                        {
                            last_color_cell.row = row;
                            last_color_cell.column = column;
                            free(colors);
                            return last_color_cell;
                        }
                    }
                }
            }
        }
    }

    free(colors);
    return last_color_cell;
}

static bool QueensSolver_IsBoardValid(QueensBoard_Board_t* board)
{
    /* check if there are rows/columns that have been entirely eliminated */
    for (uint8 rc = 0; rc < board->board_size; rc++)
    {
        uint8 eliminated_count_row = 0u;
        uint8 eliminated_count_column = 0u;

        for (uint8 i = 0; i < board->board_size; i++)
        {
            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(rc, i, board->board_size)]) == true)
            {
                eliminated_count_row++;
            }

            if (QueensBoard_IsCellEmptyPlayer(board->board[IDX(i, rc, board->board_size)]) == true)
            {
                eliminated_count_column++;
            }
        }

        if ((eliminated_count_row == board->board_size) ||
            (eliminated_count_column == board->board_size))
        {
            return false;
        }
    }

    /* check if there are colors that have been fully eliminated */
    uint8* empty_cells_color_count = (uint8*)calloc(board->board_size+1, sizeof(uint8));
    if (empty_cells_color_count == NULL)
    {
        return false;
    }

    for (uint8 row = 0; row < board->board_size; row++)
    {
        for (uint8 column = 0; column < board->board_size; column++)
        {
            uint8 color = QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]);

            if ((QueensBoard_IsCellEmptyPlayer(board->board[IDX(row, column, board->board_size)]) == true) ||
                (QueensBoard_IsPlayerQueenPresent(board->board[IDX(row, column, board->board_size)]) == true))
            {
                empty_cells_color_count[color]++;
            }
        }
    }

    for (uint8 color = 1; color <= board->board_size; color++)
    {
        if (empty_cells_color_count[color] == 0)
        {
            free(empty_cells_color_count);
            return false;
        }
    }

    free(empty_cells_color_count);
    return true;
}

/* example: QueensPermutations_RoundUpDiv(15, 2) == 8 */
static inline uint8 QueensSolver_RoundUpDiv(uint8 num, uint8 div)
{
    return ((num + div - 1u) / div);
}

const char* QueensSolver_GetStrategyName(QueensSolver_Strategy_t strategy)
{
    switch(strategy)
    {
        #define QUEENS_SOLVER_STRATEGY_CASE(name, value) \
            case value: \
                return #name;

        XMACRO_QUEENS_SOLVER_STRATEGIES(QUEENS_SOLVER_STRATEGY_CASE)

        default:
            return "Unknown strategy";
    }
}
