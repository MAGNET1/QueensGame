#include <queens_permutations.h>
#include <stdlib.h>
#include <string.h>

static QueensPermutations_Result_t QueensPermutations_Generate(u8 board_size)
{
    QueensPermutations_Result_t result;
    u32 board_alloc_count = board_size*board_size; /* worst case */
    result.boards_count = 1u; /* one empty board */
    result.boards = malloc(sizeof(QueensPermutations_QueenPlaced_t) * board_alloc_count*board_size); /* worst case TODO EDIT */
    memset(result.boards, 0u, board_alloc_count*board_size);


    for (size_t board_idx = 0u; board_idx < result.boards_count; board_idx++)
    {
        for (size_t column_idx = 0u; column_idx < board_size; column_idx++)
        {
            for (size_t row_idx = 0u; row_idx < board_size; row_idx++)
            {

            }
        }
    }
}

/* TODO
- QueensPermutations_QueenPlaced_t -> QueensPermutations_QueenIndex_t i przerobic implementacje pod to
- result.boards = board_size^board_size / 2 (przy board_size = 10 robia sie juz pojebane liczby, moze da sie to jakoś uproscic. Mozna np. obsolete boardy z poprzednich iteracji czyscic)
- bierzemy kazda plansze i idziemy kolumnami w dol. Jak PlacementLegal, tworzymy nowa plansze z postawiona krolowa i idziemy dalej. Po calej petli discardujemy poprzednie plansze i powtarzamy na nowo wygenerowanych
- wydaje mi sie ze te nowe boardy nie beda rosly tak wykladniczo, bo wiekszosc bedzie discardowana. Trzeba sprawdzic jak to sie zachowuje na board_size = 5 i dostosowac odpowiednio malloca
- pod koniec iteracji glownej mozna robic memmove nowych boardow na poczatek + memset na zera reszty

- QueensPermutations_Get(u8 board_size)
- static QueensPermutations_SaveToFile
- static QueensPermutations_LoadFromFile
*/


static inline size_t QueensPermutations_CalculateIndex(u8 board_size, size_t column, size_t row)
{
    return column*board_size + row;
}

static bool QueensPermutations_IsQueenPlacementLegal(QueensPermutations_QueenPlaced_t* board, u8 board_size, size_t column, size_t row)
{
    for (size_t row_column_idx = 0u; row_column_idx < board_size; row_column_idx++)
    {
        if ((board[QueensPermutations_CalculateIndex(board_size, column, row_column_idx)] == true) ||
            (board[QueensPermutations_CalculateIndex(board_size, row_column_idx, row)]    == true))
        {
            return false;
        }
    }
    return true;
}