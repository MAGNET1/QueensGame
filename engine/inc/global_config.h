#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#include <basic_types.h>

typedef struct
{
    /* General */
    bool debug_print_enabled;
    /* QueensPermutations */
    bool permutations_compressed;

    /* QueensBoardGen */
    uint8 boardgen_cell_skip_chance;
    uint8 boardgen_neighbor_skip_chance;
    uint8 boardgen_only_horizontal_neighbor_chance;
    uint8 boardgen_only_vertical_neighbor_chance;

    /* QueensBoard */
    bool board_sparse_print;
} Queens_GlobalConfig_t;

extern Queens_GlobalConfig_t global_config;

#endif /* GLOBAL_CONFIG_H */
