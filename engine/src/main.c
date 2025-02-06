#include <stdio.h>
#include <stdlib.h>

#include <board.h>
#include <queens_permutations.h>
#include <debug_print.h>
#include <constants.h>
#include <global_config.h>
#include <rng.h>

QueensGame_Board_t QueensGame_Board = { 0u };

int main(int argc, char** argv)
{
    debug_print_enabled = true;

    if (argc != 2)
    {
        printf("Invalid number of arguments! Expected \"./app <board_size>\"\n");
        return 1;
    }

    RNG_Seed((uint64)time(NULL));

    global_config.permutations_compressed = true;

    QueensGame_Board.board_size = (uint8)atoi(argv[1]);

    const QueensPermutations_Result_t result = QueensPermutations_GetRandom(QueensGame_Board.board_size);

    QueensPermutations_PrintBoards(&result, false);

    QueensPermutations_FreeResult(&result);

    /*RNG_Seed((uint64)time(NULL));
    for (int i = 0; i < 20; i++)
    {
        printf("%u\n", RNG_RandomRange_u32(2, 10));
    }*/

    return 0;
}
