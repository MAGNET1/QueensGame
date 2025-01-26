#include <queens_permutations.h>
#include <assert_msg.h>
#include <stdlib.h>
#include <string.h>
#include <debug_print.h>
#include <global_config.h>

/* estimated empirically */
constexpr uint32 BOARDS_INIT_MALLOC_FACTOR = 500u;

const char* filename = "QueensPermutations_XXX.bin";
constexpr uint8 filename_strlen = 26u;
constexpr uint8 filename_first_X_pos = 19u;
constexpr uint8 filename_second_X_pos = 20u;
constexpr uint8 filename_third_X_pos = 21u;

QueensPermutations_Result_t QueensPermutations_Get(const QueensPermutation_BoardSize_t board_size);
static QueensPermutations_Result_t QueensPermutations_Generate(const QueensPermutation_BoardSize_t board_size);
static bool QueensPermutations_IsQueenPlacementLegal(const QueensPermutations_QueenRowIndex_t* const board, const QueensPermutation_BoardSize_t board_size, const sint8 dest_column, const sint8 dest_row);
static bool QueensPermutations_SaveToFile(const QueensPermutations_Result_t* const result);
static QueensPermutations_Result_t QueensPermutations_LoadFromFile(const QueensPermutation_BoardSize_t board_size);
static inline void QueensPermutation_PrepareFilename(char* str, const QueensPermutation_BoardSize_t board_size);
static void QueensPermutations_Decompress(QueensPermutations_Result_t* result);
static inline uint32 QueensPermutations_RoundUpDiv(uint32 num, uint32 div);

/* TODO
- add command ctrl+c to karabiner-elements
- add undo/redo
*/

QueensPermutations_Result_t QueensPermutations_Get(const QueensPermutation_BoardSize_t board_size)
{
    QueensPermutations_Result_t result = { 0 };

    if ((board_size < QUEENS_MIN_BOARD_SIZE) ||
        (board_size > QUEENS_MAX_BOARD_SIZE))
    {
        result.success = false;
        return result;
    }

    ASSERT_MSG(sizeof(QueensPermutations_QueenRowIndex_t) == 1u, "QueensPermutations_QueenRowIndex_t must be of size 1!");

    /* check if file with board permutation has already been generated */
    /* if yes, load from file. If no, generate new one and save to file */

    /* prepare filename */
    char destination_filename[filename_strlen];
    QueensPermutation_PrepareFilename(destination_filename, board_size);

    FILE *file = fopen(destination_filename, "rb");
    if (file != NULL)
    {
        /* file exists */
        fclose(file);
        result = QueensPermutations_LoadFromFile(board_size);
    }
    else
    {
        /* file doesn't exist */
        result = QueensPermutations_Generate(board_size);
        bool write_success = QueensPermutations_SaveToFile(&result);
        assert(write_success == true);
    }

    return result; /* has to be freed by the caller (QueensPermutations_FreeResult) */
}

static bool QueensPermutations_SaveToFile(const QueensPermutations_Result_t* const result)
{
    if (result == NULL)
    {
        ASSERT_MSG(false, "null ptr");
        return false;
    }

    /* prepare filename */
    char destination_filename[filename_strlen];
    QueensPermutation_PrepareFilename(destination_filename, result->board_size);

    FILE *file = fopen(destination_filename, "wb");
    if (file == NULL)
    {
        ASSERT_MSG(false, "could not open a file");
        return false;
    }

    /* first write how many boards are there */
    fwrite(&result->boards_count, sizeof(result->boards_count), 1, file);

    /* then the actual boards */
    if (global_config.permutations_compressed == true)
    {
        constexpr size_t BUFFER_SIZE = 128u;
        QueensPermutations_QueenRowIndex_t buffer[BUFFER_SIZE] = { 0u };
        bool lower_nibble = false;
        size_t buffer_idx = 0u;

        for (size_t column_idx = 0u; column_idx < (result->board_size*result->boards_count); column_idx++)
        {

            if (lower_nibble == false)
            {
                buffer[buffer_idx] |= (result->boards[column_idx] << NIBBLE_LEN);
            }
            else
            {
                buffer[buffer_idx] |= (result->boards[column_idx] & 0b1111);
                buffer_idx++;

                if (buffer_idx >= BUFFER_SIZE)
                {
                    fwrite(buffer, sizeof(QueensPermutations_QueenRowIndex_t), BUFFER_SIZE, file);
                    memset(buffer, 0, BUFFER_SIZE);
                    buffer_idx = 0u;
                }
            }

            lower_nibble = !lower_nibble;
        }

        /* include last number if stored only in higher nibble */
        if (lower_nibble == true)
        {
            buffer_idx++;
        }

        fwrite(buffer, sizeof(QueensPermutations_QueenRowIndex_t), buffer_idx, file);

    }
    else
    {
        fwrite(result->boards, sizeof(QueensPermutations_QueenRowIndex_t), result->board_size*result->boards_count, file);
    }


    fclose(file);

    return true;
}

static QueensPermutations_Result_t QueensPermutations_LoadFromFile(const QueensPermutation_BoardSize_t board_size)
{
    QueensPermutations_Result_t result;
    result.board_size = board_size;
    result.success = false;

    /* prepare filename */
    char destination_filename[filename_strlen];
    QueensPermutation_PrepareFilename(destination_filename, board_size);

    FILE *file = fopen(destination_filename, "rb");
    if (file == NULL)
    {
        ASSERT_MSG(false, "could not open a file");
        return result;
    }

    /* retrieve number of boards */
    fread(&result.boards_count, sizeof(result.boards_count), 1, file);
    const size_t single_board_alloc_size = sizeof(QueensPermutations_QueenRowIndex_t) * board_size;
    result.boards = malloc(single_board_alloc_size*result.boards_count);
    if (result.boards == NULL)
    {
        ASSERT_MSG(false, "malloc error");
        return result;
    }

    fread(result.boards, sizeof(QueensPermutations_QueenRowIndex_t), result.board_size*result.boards_count, file);

    fclose(file);
    result.success = true;

    if (global_config.permutations_compressed == true)
    {
        QueensPermutations_Decompress(&result);
    }

    return result;
}

static void QueensPermutations_Decompress(QueensPermutations_Result_t* result)
{
    /* function assumes that result->boards has enough size for decompressed array */

    /* start from the end to avoid temporaries */
    const size_t last_column_idx = (result->board_size*result->boards_count);
    size_t column_compressed_idx = QueensPermutations_RoundUpDiv((uint32)last_column_idx, 2u) - 1;
    bool lower_nibble = true;
    /* if last number contains two columns, start from lower nibble. Otherwise, start from higher nibble */
    if ((column_compressed_idx - (last_column_idx/2u)) == 0u)
    {
        lower_nibble = false;
    }

    for (size_t column_idx = last_column_idx; column_idx-- > 0u;)
    {
        if (lower_nibble == true)
        {
            result->boards[column_idx] = (result->boards[column_compressed_idx] & 0b1111);
        }
        else
        {
            result->boards[column_idx] = (result->boards[column_compressed_idx] >> NIBBLE_LEN);
            column_compressed_idx--;
        }

        lower_nibble = !lower_nibble;
    }

}

static QueensPermutations_Result_t QueensPermutations_Generate(const QueensPermutation_BoardSize_t board_size)
{
    QueensPermutations_Result_t result;
    result.success = false;
    result.board_size = board_size;
    result.boards_count = 1u; /* one empty board */
    uint32 new_board_candidates = 0u;
    const size_t single_board_alloc_size = sizeof(QueensPermutations_QueenRowIndex_t) * board_size;
    uint64 boards_capacity = single_board_alloc_size * board_size * BOARDS_INIT_MALLOC_FACTOR;
    result.boards = malloc(boards_capacity);

    if (result.boards == NULL)
    {
        ASSERT_MSG(false, "malloc error");
        return result;
    }

    memset(result.boards, QUEEN_ROW_NOT_EXISTS, boards_capacity);

    /*
        algorithm:
        1. Start with single, empty board
        2. on every column iteration, attempt placing queen in every row
        3. If placement is legal, create a copy of parent board with newly added queen and append new board at the end of the list
        4. Move newly created boards at the beginning of the list, get rid of older iterations
    */
    for (sint8 column_idx = 0; column_idx < board_size; column_idx++)
    {
        new_board_candidates = 0u;

        for (size_t board_idx = 0u; board_idx < result.boards_count; board_idx++)
        {
            for (sint8 row_idx = 0; row_idx < board_size; row_idx++)
            {

                if (QueensPermutations_IsQueenPlacementLegal(&result.boards[board_size*board_idx], board_size, column_idx, row_idx) == true)
                {
                    while((board_size*(result.boards_count+new_board_candidates) + board_size) >= boards_capacity)
                    {
                        /* no more space for new boards, increase boards capacity 2x */
                        boards_capacity *= 2u;
                        result.boards = (QueensPermutations_QueenRowIndex_t*)realloc(result.boards, boards_capacity);
                        if (result.boards == NULL)
                        {
                            ASSERT_MSG(false, "realloc error");
                            return result;
                        }
                        memset(&result.boards[boards_capacity/2], QUEEN_ROW_NOT_EXISTS, boards_capacity/2);
                    }

                    /* create new board at the end of boards list */
                    memmove(&result.boards[board_size*(result.boards_count+new_board_candidates)], &result.boards[board_size*board_idx], single_board_alloc_size);

                    /* add queen to new board */
                    result.boards[(uint32)board_size*(result.boards_count+new_board_candidates) + (uint32)column_idx] = row_idx;
                    ++new_board_candidates;
                }
            }
        }

        /* move all the elements from this iteration to the beginning and discard old ones */
        memmove(result.boards, &result.boards[board_size*result.boards_count], single_board_alloc_size*new_board_candidates);
        result.boards_count = new_board_candidates;
        new_board_candidates = 0u;
    }


    /* algorithm done, get rid of redundant memory */
    result.boards = (QueensPermutations_QueenRowIndex_t*)realloc(result.boards, single_board_alloc_size*result.boards_count);

    if (result.boards == NULL)
    {
        ASSERT_MSG(false, "realloc error");
        return result;
    }

    result.success = true;

    return result;
}

bool QueensPermutations_FreeResult(const QueensPermutations_Result_t result)
{
    if ((result.boards != NULL) &&
        (result.success == true))
    {
        free(result.boards);
    }

    return true;
}

static bool QueensPermutations_IsQueenPlacementLegal(const QueensPermutations_QueenRowIndex_t* const board, const QueensPermutation_BoardSize_t board_size, const sint8 dest_column, const sint8 dest_row)
{
    ASSERT_MSG(dest_column >= 0,         "dest_column: %d", dest_column);
    ASSERT_MSG(dest_column < board_size, "dest_column: %d", dest_column);
    ASSERT_MSG(dest_row    >= 0,         "dest_row: %d",    dest_row);
    ASSERT_MSG(dest_row    < board_size, "dest_row: %d",    dest_row);

    /* no queen in the same column */
    if (board[dest_column] != QUEEN_ROW_NOT_EXISTS)
    {
        return false;
    }

    /* no queen in the same row */
    for (sint8 column_idx = 0; column_idx < board_size; column_idx++)
    {
        if (board[column_idx] == dest_row)
        {
            return false;
        }
    }

    /* no queens in the neighboring diagonals */
    for (sint8 column_idx = dest_column - 1; column_idx <= dest_column + 1; column_idx++)
    {
        if (column_idx < 0 || column_idx >= board_size)
        {
            continue; // Skip out-of-bounds columns
        }

        sint8 existing_row = board[column_idx];
        if (existing_row != QUEEN_ROW_NOT_EXISTS)
        {
            /* Check if the neighboring square is diagonally adjacent */
            if (abs(existing_row - dest_row) == 1) // Difference in row is 1
            {
                return false;
            }
        }
    }

    return true;
}

static inline void QueensPermutation_PrepareFilename(char* str, const QueensPermutation_BoardSize_t board_size)
{
    strcpy(str, filename);
    str[filename_first_X_pos]  = (char)(board_size/10u) + '0';
    str[filename_second_X_pos] = (char)(board_size%10u) + '0';

    if (global_config.permutations_compressed == true)
    {
        str[filename_third_X_pos] = 'c';
    }
    else
    {
        str[filename_third_X_pos] = 'n';
    }
}

[[maybe_unused]] void QueensPermutations_PrintBoards(const QueensPermutations_Result_t* const result, bool as_hex)
{
    printf("Board size: %d\nBoards count: %d\n\n", result->board_size, result->boards_count);

    for (uint32 board_idx = 0u; board_idx < result->boards_count; board_idx++)
    {
        for (uint8 board_column_idx = 0u; board_column_idx < result->board_size; board_column_idx++)
        {
            if (as_hex == true)
            {
                printf("%02X ", result->boards[(board_idx*result->board_size) + board_column_idx]);
            }
            else
            {
                printf("%d ", result->boards[(board_idx*result->board_size) + board_column_idx]);
            }
        }
        printf("\n");
    }
}

/* example: QueensPermutations_RoundUpDiv(15, 2) == 8 */
static inline uint32 QueensPermutations_RoundUpDiv(uint32 num, uint32 div)
{
    return ((num + div - 1u) / div);
}
