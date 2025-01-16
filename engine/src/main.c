#include <stdio.h>
#include <stdlib.h>
#include <board.h>

QueensGame_Board_t QueensGame_Board = { 0u };

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Invalid number of arguments! Expected \"./app <board_size>\"\n");
        return 1;
    }

    QueensGame_Board.board_size = atoi(argv[1]);
    QueensGame_Board.elements = (QueensGame_Element_t*)malloc(sizeof(*QueensGame_Board.elements) * QueensGame_Board.board_size);

    return 0;
}
