#include <queens_boardgen.h>
#include <global_config.h>
#include <rng.h>
#include <assert.h>
#include <string.h>

#include <stdlib.h>

static uint8 QueensBoardGen_GetCellNeighbors(const QueensBoard_Board_t* board, const uint8 row, const uint8 column, int neighbors[4][2], bool only_horizontal, bool only_vertical);

QueensBoardGen_Result_t QueensBoardGen_Generate(QueensBoard_Board_t* board, QueensPermutations_Result_t* permutation)
{
    QueensBoardGen_Result_t result = QUEENS_BOARDGEN_ERROR;

    if (board->board_size < QUEENS_MIN_BOARD_SIZE || board->board_size > QUEENS_MAX_BOARD_SIZE)
    {
        return result;
    }

    bool permutation_provided_externally = true;
    QueensPermutations_Result_t random_permutation = { 0 };

    /* no permutation provided externally, create new one */
    if (permutation == NULL)
    {
        random_permutation = QueensPermutations_GetRandom(board->board_size);
        permutation = &random_permutation;
        permutation_provided_externally = false;
    }
    else
    {
        if (permutation->success == false)
        {
            free(board->board);
            return result;
        }

        if (permutation->board_size != board->board_size)
        {
            free(board->board);
            return result;
        }

        if (permutation->boards_count != 1u)
        {
            free(board->board);
            return result;
        }
    }

    if (permutation->success == false)
    {
        free(board->board);
        return result;
    }

    QueensBoard_ZeroeBoard(board);

    /* Place a queen and apply a color */
    for (uint8 row = 0; row < board->board_size; row++)
    {
        board->board[permutation->boards[row]*board->board_size + row] = (row+1u);
    }

    if (permutation_provided_externally == false)
    {
        (void)QueensPermutations_FreeResult(permutation);
    }

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

bool QueensBoardGen_ValidateOnlyOneSolution(const QueensBoard_Board_t* board, const QueensPermutations_Result_t* permutations)
{
    assert(board != NULL);
    assert(board->board != NULL);

    QueensPermutations_Result_t all_permutations = { 0 };

    /* check if permutations were provided externally */
    if (permutations == NULL)
    {
        all_permutations = QueensPermutations_GetAll(board->board_size);
        permutations = &all_permutations;
    }
    else
    {
        if (permutations->board_size != board->board_size)
        {
            return false;
        }
    }


    if (permutations->success == false)
    {
        return false;
    }

    /* For each permutation, check if each queen has a unique color. There has to be exactly one permutation that meets this criteria */
    /* Create an array that stores colors and make sure it consists of unique elements */
    uint8* colors = (uint8*)calloc(board->board_size+1u, sizeof(uint8));
    if (colors == NULL)
    {
        return false;
    }

    bool one_solution = false;

    for (uint32 permutation_idx = 0; permutation_idx < permutations->boards_count; permutation_idx++)
    {
        bool valid_permutation = true;

        for (uint8 column = 0; column < board->board_size; column++)
        {
            uint8 color = QueensBoard_GetColor(board->board[IDX(permutations->boards[permutation_idx*board->board_size + column], column, board->board_size)]);
            if (colors[color] == 1u)
            {
                valid_permutation = false;
                break;
            }
            else
            {
                colors[color] = 1u;
            }
        }

        if (valid_permutation)
        {
            if (one_solution)
            {
                one_solution = false;
                break;
            }
            else
            {
                one_solution = true;
            }
        }

        memset(colors, 0, board->board_size+1);
    }

    free(colors);

    return one_solution;
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
