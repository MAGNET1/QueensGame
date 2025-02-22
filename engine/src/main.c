#include <stdio.h>
#include <stdlib.h>

#include <board.h>
#include <queens_permutations.h>
#include <queens_boardgen.h>
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
    global_config.boardgen_cell_skip_chance = 20u;
    global_config.boardgen_neighbor_skip_chance = 20u;
    global_config.boardgen_only_horizontal_neighbor_chance = 80u;
    global_config.boardgen_only_vertical_neighbor_chance = 80u;

    QueensGame_Board.board_size = (uint8)atoi(argv[1]);

    QueensBoard_Board_t board = { 0u };
    board.board_size = QueensGame_Board.board_size;

    volatile int n = 0;

    do
    {
        QueensBoardGen_Generate(&board);
        n++;
    } while (QueensBoardGen_ValidateOnlyOneSolution(&board) == false);

    QueensBoardGen_PrintBoard(&board);

    printf("Number of iterations: %d\n", n);

    return 0;
}
