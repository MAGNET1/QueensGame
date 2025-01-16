#include <basic_types.h>
#include <stdbool.h>

typedef bool QueensPermutations_QueenPlaced_t;

typedef struct
{
    u8 boards_count;
    QueensPermutations_QueenPlaced_t* boards; /* columns go first */
} QueensPermutations_Result_t;