#include <global_config.h>
#include <rng.h>
#include <arg_parser.h>

void global_config_init();

int main(int argc, char** argv)
{
    RNG_Seed((uint64)time(NULL));
    // RNG_Seed((uint64)4ULL);
    global_config_init();

    return ArgParser_ParseArguments(argc, argv);
}

void global_config_init()
{
    global_config.debug_print_enabled = false;
    global_config.permutations_compressed = true;
    global_config.boardgen_cell_skip_chance = 20u;
    global_config.boardgen_neighbor_skip_chance = 80u;
    global_config.boardgen_only_horizontal_neighbor_chance = 5u;
    global_config.boardgen_only_vertical_neighbor_chance = 5u;
    global_config.board_sparse_print = false;
}
