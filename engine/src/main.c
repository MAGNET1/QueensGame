#include <stdio.h>
#include <stdlib.h>

#include <board.h>
#include <queens_permutations.h>
#include <debug_print.h>

QueensGame_Board_t QueensGame_Board = { 0u };

int main(int argc, char** argv)
{
    debug_print_enabled = true;

    if (argc != 2)
    {
        printf("Invalid number of arguments! Expected \"./app <board_size>\"\n");
        return 1;
    }

    QueensGame_Board.board_size = atoi(argv[1]);
    /* QueensGame_Board.elements = (QueensGame_Element_t*)malloc(sizeof(*QueensGame_Board.elements) * QueensGame_Board.board_size); */

    QueensPermutations_Result_t result = QueensPermutations_Generate(QueensGame_Board.board_size);

    printf("Boards: %d %d\n", QueensGame_Board.board_size, result.boards_count);

    for (size_t board_idx = 0u; board_idx < result.boards_count; board_idx++)
    {
        printf("\n\n");
        for (size_t board_field_idx = 0u; board_field_idx < QueensGame_Board.board_size; board_field_idx++)
        {
            printf("%d ", result.boards[board_idx*QueensGame_Board.board_size + board_field_idx]);
        }
    }

    QueensPermutations_FreeResult(result);

    return 0;
}
