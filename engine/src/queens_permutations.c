#include <queens_permutations.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <debug_print.h>
#include <global_config.h>
#include <rng.h>

/* estimated empirically */
constexpr uint32 BOARDS_INIT_MALLOC_FACTOR = 500u;

const char* QueensPermutations_filename = "QueensPermutations_XXX.bin";
constexpr uint8 QueensPermutations_filename_strlen = 27u;
constexpr uint8 QueensPermutations_filename_first_X_pos = 19u;
constexpr uint8 QueensPermutations_filename_second_X_pos = 20u;
constexpr uint8 QueensPermutations_filename_third_X_pos = 21u;

static QueensPermutations_Result_t QueensPermutations_Generate(const QueensPermutation_BoardSize_t board_size);
static bool QueensPermutations_IsQueenPlacementLegal(const QueensPermutations_QueenRowIndex_t* const board, const QueensPermutation_BoardSize_t board_size, const sint8 dest_column, const sint8 dest_row);
static bool QueensPermutations_SaveToFile(const QueensPermutations_Result_t* const result);
static QueensPermutations_Result_t QueensPermutations_LoadAllFromFile(const QueensPermutation_BoardSize_t board_size);
static void QueensPermutations_Decompress(QueensPermutations_Result_t* result);
static inline uint32 QueensPermutations_RoundUpDiv(uint32 num, uint32 div);
static FILE* QueensPermutations_OpenPermutationsFile(QueensPermutation_BoardSize_t board_size, const char* mode);

[[maybe_unused]] QueensPermutations_Result_t QueensPermutations_GetRandom(const QueensPermutation_BoardSize_t board_size)
{
    QueensPermutations_Result_t result = { 0 };

    if ((board_size < QUEENS_MIN_BOARD_SIZE) ||
        (board_size > QUEENS_MAX_BOARD_SIZE))
    {
        result.success = false;
        return result;
    }

    assert(sizeof(QueensPermutations_QueenRowIndex_t) == 1u);

    /* check if file with board permutations has already been generated */
    /* if not, create it first */

    FILE *file = QueensPermutations_OpenPermutationsFile(board_size, "rb");
    if (file == NULL)
    {
        /* file doesn't exist */
        result = QueensPermutations_Generate(board_size);

        bool write_success = QueensPermutations_SaveToFile(&result);
        assert(write_success == true);

        bool free_success = QueensPermutations_FreeResult(&result);
        assert(free_success == true);

        /* now that permutations are generated, open file again */
        file = QueensPermutations_OpenPermutationsFile(board_size, "rb");
        assert(file != NULL);
    }

    result.board_size = board_size;
    result.success = false;
    result.boards_count = 1u;

    result.boards = malloc(board_size * sizeof(QueensPermutations_QueenRowIndex_t));
    if (result.boards == NULL)
    {
        assert(false);
        return result;
    }

    uint32 file_boards_count;
    fread(&file_boards_count, sizeof(file_boards_count), 1, file);

    uint32 random_board_offset = 0u;

    if (global_config.permutations_compressed == true)
    {
        uint32 file_elements_read_count = QueensPermutations_RoundUpDiv(board_size, 2u);
        uint32 random_board_num = RNG_RandomRange_u32(0u, file_boards_count-1u);
        random_board_offset = (uint32)sizeof(file_boards_count) + random_board_num * (uint32)board_size / 2u;

        /* scenario where board size is odd */
        bool board_starts_from_middle_of_byte = ((random_board_num % 2u) != 0u);

        /* for even board sizes, this is always false */
        if (board_size % 2u == 0u)
        {
            board_starts_from_middle_of_byte = false;
        }

        fseek(file, random_board_offset, SEEK_SET);
        fread(result.boards, sizeof(QueensPermutations_QueenRowIndex_t), file_elements_read_count, file);

        if (board_starts_from_middle_of_byte == true)
        {
            /* board starts from the middle of a byte, shift all elements one nibble to the left for proper decompression alignment */
            for (size_t i = 0; i < board_size - 1; i++) {
                result.boards[i] = (QueensPermutations_QueenRowIndex_t)(
                    ((uint8_t)result.boards[i] << NIBBLE_LEN) | ((uint8_t)result.boards[i + 1] >> NIBBLE_LEN)
                );
            }
            result.boards[board_size - 1] = (QueensPermutations_QueenRowIndex_t)(
                (uint8_t)result.boards[board_size - 1] << NIBBLE_LEN
            );

        }

        QueensPermutations_Decompress(&result);
    }
    else
    {
        /* first 4 bytes is for boards count, needs to be offset */
        random_board_offset = (uint32)sizeof(file_boards_count) + (RNG_RandomRange_u32(0u, file_boards_count-1u) * board_size);
        fseek(file, random_board_offset, SEEK_SET);
        fread(result.boards, sizeof(QueensPermutations_QueenRowIndex_t), result.board_size, file);
    }

    fclose(file);
    result.success = true;

    return result; /* has to be freed by the caller (QueensPermutations_FreeResult) */
}

[[maybe_unused]] QueensPermutations_Result_t QueensPermutations_GetAll(const QueensPermutation_BoardSize_t board_size)
{
    QueensPermutations_Result_t result = { 0 };

    if ((board_size < QUEENS_MIN_BOARD_SIZE) ||
        (board_size > QUEENS_MAX_BOARD_SIZE))
    {
        result.success = false;
        return result;
    }


    assert(sizeof(QueensPermutations_QueenRowIndex_t) == 1u);

    /* check if file with board permutation has already been generated */
    /* if yes, load from file. If no, generate new one and save to file */

    FILE *file = QueensPermutations_OpenPermutationsFile(board_size, "rb");
    if (file != NULL)
    {
        /* file exists */
        fclose(file);
        result = QueensPermutations_LoadAllFromFile(board_size);
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
        assert(false);
        return false;
    }

    FILE *file = QueensPermutations_OpenPermutationsFile(result->board_size, "wb");
    if (file == NULL)
    {
        assert(false);
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

static QueensPermutations_Result_t QueensPermutations_LoadAllFromFile(const QueensPermutation_BoardSize_t board_size)
{
    QueensPermutations_Result_t result;
    result.board_size = board_size;
    result.success = false;

    FILE *file = QueensPermutations_OpenPermutationsFile(board_size, "rb");
    if (file == NULL)
    {
        assert(false);
        return result;
    }

    /* retrieve number of boards */
    fread(&result.boards_count, sizeof(result.boards_count), 1, file);
    const size_t single_board_alloc_size = sizeof(QueensPermutations_QueenRowIndex_t) * board_size;
    result.boards = malloc(single_board_alloc_size*result.boards_count);
    if (result.boards == NULL)
    {
        assert(false);
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
    const size_t last_column_idx = (result->board_size * result->boards_count);
    size_t column_compressed_idx = QueensPermutations_RoundUpDiv((uint32)last_column_idx, 2u) - 1;
    bool lower_nibble = true;
    /* if last number contains two columns, start from lower nibble. Otherwise, start from higher nibble */
    if ((column_compressed_idx - (last_column_idx / 2u)) == 0u)
    {
        lower_nibble = false;
    }

    for (size_t column_idx = last_column_idx; column_idx-- > 0u;)
    {
        if (lower_nibble == true)
        {
            result->boards[column_idx] = (QueensPermutations_QueenRowIndex_t)(
                (uint8_t)(result->boards[column_compressed_idx] & 0b1111)
            );
        }
        else
        {
            result->boards[column_idx] = (QueensPermutations_QueenRowIndex_t)(
                (uint8_t)(result->boards[column_compressed_idx] >> NIBBLE_LEN)
            );
            column_compressed_idx--;
        }

        /* Ensure the value is correctly interpreted as unsigned */
        result->boards[column_idx] &= 0x0F;

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
        assert(false);
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
                            assert(false);
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
        assert(false);
        return result;
    }

    result.success = true;

    return result;
}

[[maybe_unused]] bool QueensPermutations_FreeResult(const QueensPermutations_Result_t* result)
{
    if ((result->boards != NULL) &&
        (result->success == true))
    {
        free(result->boards);
    }

    return true;
}

static bool QueensPermutations_IsQueenPlacementLegal(const QueensPermutations_QueenRowIndex_t* const board, const QueensPermutation_BoardSize_t board_size, const sint8 dest_column, const sint8 dest_row)
{
    assert(dest_column >= 0        );
    assert(dest_column < board_size);
    assert(dest_row    >= 0        );
    assert(dest_row    < board_size);

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

static FILE* QueensPermutations_OpenPermutationsFile(QueensPermutation_BoardSize_t board_size, const char* mode)
{
    /* prepare filename */
    char destination_filename[QueensPermutations_filename_strlen];
    strcpy(destination_filename, QueensPermutations_filename);

    /* compose board size into filename */
    destination_filename[QueensPermutations_filename_first_X_pos]  = (char)(board_size/10u) + '0';
    destination_filename[QueensPermutations_filename_second_X_pos] = (char)(board_size%10u) + '0';

    if (global_config.permutations_compressed == true)
    {
        destination_filename[QueensPermutations_filename_third_X_pos] = 'c';
    }
    else
    {
        destination_filename[QueensPermutations_filename_third_X_pos] = 'n';
    }


    return fopen(destination_filename, mode);
}

/* example: QueensPermutations_RoundUpDiv(15, 2) == 8 */
static inline uint32 QueensPermutations_RoundUpDiv(uint32 num, uint32 div)
{
    return ((num + div - 1u) / div);
}
