#include <basic_types.h>

typedef uint8 QueensGame_Element_t;

typedef struct
{
    uint8 board_size;
    QueensGame_Element_t* elements;
} QueensGame_Board_t;
