#include <queens_boardgen.h>
#include <queens_permutations.h>
#include <global_config.h>
#include <rng.h>
#include <assert.h>

#include <stdlib.h>

static uint8 QueensBoardGen_GetCellNeighbors(const QueensBoard_Board_t* board, const uint8 row, const uint8 column, int neighbors[4][2], bool only_horizontal, bool only_vertical);

QueensBoardGen_Result_t QueensBoardGen_Generate(QueensBoard_Board_t* board)
{
    QueensBoardGen_Result_t result = QUEENS_BOARDGEN_ERROR;
    if (board == NULL)
    {
        return result;
    }

    if (board->board_size < QUEENS_MIN_BOARD_SIZE || board->board_size > QUEENS_MAX_BOARD_SIZE)
    {
        return result;
    }

    board->board = (QueensBoard_Cell_t*)calloc(board->board_size*board->board_size, sizeof(QueensBoard_Cell_t));

    if (board->board == NULL)
    {
        return result;
    }

    QueensPermutations_Result_t random_permutation = QueensPermutations_GetRandom(board->board_size);
    if (random_permutation.success == false)
    {
        free(board->board);
        return result;
    }

    QueensPermutations_PrintBoards(&random_permutation, false);

    /* Place a queen and apply a color */
    for (uint8 row = 0; row < board->board_size; row++)
    {
        board->board[random_permutation.boards[row]*board->board_size + row] = ((row+1u)|QUEEN_PRESENT);
    }

    (void)QueensPermutations_FreeResult(&random_permutation);

    uint16 non_color_cells_count = 0u;

    /* multi-pass flood fill */
    do
    {
        non_color_cells_count = 0u;

        for (uint8 row = 0; row < board->board_size; row++)
        {
            for (uint8 column = 0; column < board->board_size; column++)
            {
                if (QueensBoard_GetColor(board->board[IDX(row, column, board->board_size)]) == COLOR_NONE)
                {
                    non_color_cells_count++;

                    /* introduce randomness */
                    if (RNG_RandomRange_u32(0, 100) < global_config.boardgen_cell_skip_chance)
                    {
                        continue;
                    }

                    bool only_horizontal = (RNG_RandomRange_u32(0, 100) < global_config.boardgen_only_horizontal_neighbor_chance);
                    bool only_vertical = (RNG_RandomRange_u32(0, 100) < global_config.boardgen_only_vertical_neighbor_chance);
                    int neighbors[4][2] = { 0 };
                    uint8 neighbors_count = QueensBoardGen_GetCellNeighbors(board, row, column, neighbors, only_horizontal, only_vertical);

                    for (uint8 i = 0; i < neighbors_count; i++)
                    {
                        if (RNG_RandomRange_u32(0, 100) < global_config.boardgen_neighbor_skip_chance)
                        {
                            continue;
                        }

                        if (QueensBoard_GetColor(board->board[IDX(neighbors[i][0], neighbors[i][1], board->board_size)]) != COLOR_NONE)
                        {
                            QueensBoard_SetColor(&board->board[IDX(row, column, board->board_size)], QueensBoard_GetColor(board->board[IDX(neighbors[i][0], neighbors[i][1], board->board_size)]));
                            break;
                        }
                    }
                }
            }
        }
    }
    while (non_color_cells_count > 0u);

    result = QUEENS_BOARDGEN_SUCCESS;

    return result;
}

static uint8 QueensBoardGen_GetCellNeighbors(const QueensBoard_Board_t* board, const uint8 row, const uint8 column, int neighbors[4][2], bool only_horizontal, bool only_vertical)
{
    int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    uint8 neighbors_count = 0u;

    uint8 start_idx = (only_horizontal ? 2 : 0);
    uint8 end_idx = (only_vertical ? 2 : 4);

    for (uint8 i = start_idx; i < end_idx; i++)
    {
        int new_row = row + directions[i][0];
        int new_column = column + directions[i][1];

        if (new_row >= 0 && new_row < board->board_size &&
            new_column >= 0 && new_column < board->board_size)
        {
            neighbors[neighbors_count][0] = new_row;
            neighbors[neighbors_count][1] = new_column;
            neighbors_count++;
        }
    }

    return neighbors_count;
}

void QueensBoardGen_PrintBoard(const QueensBoard_Board_t* const board)
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
            if (QueensBoard_IsQueenPresent(cell))
            {
                printf("%s\033[1mQ\033[0m ", colors_console[color % 16]);
            }
            else
            {
                printf("%sX\033[0m ", colors_console[color % 16]);
            }
        }
        printf("\n");
    }
}
