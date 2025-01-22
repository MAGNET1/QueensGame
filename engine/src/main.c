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

    QueensGame_Board.board_size = (uint8)atoi(argv[1]);

    const QueensPermutations_Result_t result = QueensPermutations_Generate(QueensGame_Board.board_size);

    printf("Boards: %d %d\n", QueensGame_Board.board_size, result.boards_count);

    QueensPermutations_FreeResult(result);

    return 0;
}
