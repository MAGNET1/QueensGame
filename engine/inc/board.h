#include <basic_types.h>

typedef u8 QueensGame_Element_t;

typedef struct
{
    u8 board_size;
    QueensGame_Element_t* elements;
} QueensGame_Board_t;