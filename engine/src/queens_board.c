#include <queens_board.h>
#include <global_config.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <debug_print.h>

bool QueensBoard_Create(QueensBoard_Board_t* empty_board_ptr, const QueensBoard_Size_t size)
{
    empty_board_ptr->board = (QueensBoard_Cell_t*)calloc(size * size, sizeof(QueensBoard_Cell_t));
    if (empty_board_ptr->board == NULL)
    {
        return false;
    }

    empty_board_ptr->board_size = size;

    return true;
}

void QueensBoard_Free(QueensBoard_Board_t* board)
{
    free(board->board);
}

void QueensBoard_ZeroeBoard(QueensBoard_Board_t* board)
{
    memset(board->board, 0, sizeof(QueensBoard_Cell_t) * board->board_size * board->board_size);
}

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

/* queen - solution, it's certain that it's a valid one */
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

/* queen placed by the player */
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

void QueensBoard_PrintBoard(const QueensBoard_Board_t* const board)
{
    const char* colors_console[] = {
        "\033[0m", /* COLOR_NONE */
        "\033[31m", /* COLOR_RED */
        "\033[32m", /* COLOR_GREEN */
        "\033[33m", /* COLOR_YELLOW */
        "\033[34m", /* COLOR_BLUE */
        "\033[35m", /* COLOR_MAGENTA */
        "\033[36m", /* COLOR_CYAN */
        "\033[37m", /* COLOR_WHITE */
        "\033[90m", /* COLOR_BRIGHT_BLACK */
        "\033[91m", /* COLOR_BRIGHT_RED */
        "\033[92m", /* COLOR_BRIGHT_GREEN */
        "\033[93m", /* COLOR_BRIGHT_YELLOW */
        "\033[94m", /* COLOR_BRIGHT_BLUE */
        "\033[95m", /* COLOR_BRIGHT_MAGENTA */
        "\033[96m", /* COLOR_BRIGHT_CYAN */
        "\033[97m"  /* COLOR_BRIGHT_WHITE */
    };

    assert(board != NULL);
    assert(board->board != NULL);

    for (uint8 row = 0; row < board->board_size; row++)
    {
        for (uint8 column = 0; column < board->board_size; column++)
        {
            QueensBoard_Cell_t cell = board->board[IDX(row, column, board->board_size)];
            uint8 color = QueensBoard_GetColor(cell);
            if (QueensBoard_IsPlayerQueenPresent(cell))
            {
                debug_print("%sQ\033[0m %s", colors_console[color % 16], global_config.board_sparse_print ? "  " : "");
            }
            else if (QueensBoard_IsCellEliminated(cell))
            {
                debug_print("%s\033[1mX\033[0m %s", colors_console[color % 16], global_config.board_sparse_print ? "  " : "");
            }
            else
            {
                debug_print("%s▓\033[0m %s", colors_console[color % 16], global_config.board_sparse_print ? "  " : "");
            }
        }
        debug_print("\n%s", global_config.board_sparse_print ? "\n" : "");
    }
}

void QueensBoard_PrintBoardAsString(const QueensBoard_Board_t* board)
{
    assert(board != NULL);
    assert(board->board != NULL);

    printf("%02u|%02X", board->board_size, board->board[0]);

    for (uint8 element_idx = 1u; element_idx < board->board_size * board->board_size; element_idx++)
    {
        printf(",%02X", board->board[element_idx]);
    }
}

bool QueensBoard_ParseFromString(const char* board_str, QueensBoard_Board_t* board)
{
    assert(board_str != NULL);
    assert(board != NULL);

    char board_str_copy[QUEENS_MAX_BOARD_SIZE * QUEENS_MAX_BOARD_SIZE * 3 + 3 + 1] = {0};
    strncpy(board_str_copy, board_str, sizeof(board_str_copy) - 1);
    char *token = strtok(board_str_copy, "|");
    if (token == NULL)
    {
        return false;
    }

    board->board_size = (QueensBoard_Size_t)atoi(token);
    if (board->board_size < QUEENS_MIN_BOARD_SIZE || board->board_size > QUEENS_MAX_BOARD_SIZE)
    {
        return false;
    }

    board->board = (QueensBoard_Cell_t*)calloc(board->board_size * board->board_size, sizeof(QueensBoard_Cell_t));

    if (board->board == NULL)
    {
        return false;
    }

    uint8 n = 0u;
    token = strtok(NULL, ",");
    while (token != NULL)
    {
        board->board[n++] = (QueensBoard_Cell_t)strtol(token, NULL, 16);
        if (n > board->board_size * board->board_size)
        {
            free(board->board);
            return false;
        }
        token = strtok(NULL, ",");
    }

    if (n != board->board_size * board->board_size)
    {
        free(board->board);
        return false;
    }

    return true;
}

