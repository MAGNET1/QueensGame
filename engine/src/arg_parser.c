#include <arg_parser.h>
#include <debug_print.h>
#include <constants.h>
#include <queens_permutations.h>
#include <queens_boardgen.h>
#include <queens_solver.h>
#include <string.h>
#include <stdlib.h>

typedef int (*ArgParser_Command_t)(int, char **, size_t);

typedef struct
{
    const char *command;
    ArgParser_Command_t function;
    const char *description;
    const char *args;
} ArgParser_Commands_t;

int ArgParser_Version(int argc, char **argv, size_t command_idx);
int ArgParser_Help(int argc, char **argv, size_t command_idx);
int ArgParser_Generate(int argc, char **argv, size_t command_idx);
int ArgParser_GenerateAndSolve(int argc, char **argv, size_t command_idx);
int ArgParser_SolveStep(int argc, char **argv, size_t command_idx);
int ArgParser_PrintFromString(int argc, char **argv, size_t command_idx);

ArgParser_Commands_t commands[] = {
    {"--help",               ArgParser_Help,             "Show help",          ""},
    {"--version",            ArgParser_Version,          "Show version",       ""},
    {"--generate",           ArgParser_Generate,         "Generate new board", "<board_size>"},
    {"--generate_and_solve", ArgParser_GenerateAndSolve, "Generate new board and show solving process", "<board_size>"},
    {"--solve_step",         ArgParser_SolveStep,        "Returns board with one new solving step", "<board_string>"},
    {"--print_from_string",  ArgParser_PrintFromString,  "Prints board from board string", "<board_string>"},
    //{"--solve",    ArgParser_Solve,    "Given a map, solve it. Args: <map> <type (all/single)>"}
};

int ArgParser_ParseArguments(int argc, char **argv)
{
    if (argc < 2)
    {
        return ArgParser_Help(argc, argv, 0u);
    }

    for (uint8 i = 0; i < sizeof(commands)/sizeof(ArgParser_Commands_t); i++)
    {
        if (strcmp(argv[1], commands[i].command) == 0)
        {
            return commands[i].function(argc, argv, i);
        }
    }

    return 1;
}

int ArgParser_Help(int argc, char **argv, size_t command_idx)
{
    (void)argc;
    (void)command_idx;

    printf("Usage: %s <command> [<args>]\n", argv[0]);
    printf("Commands:\n");
    for (uint8 i = 0; i < sizeof(commands)/sizeof(ArgParser_Commands_t); i++)
    {
        printf("  %s - %s%s%s\n", commands[i].command, commands[i].description, commands[i].args[0] != '\0' ? ". Args: " : "", commands[i].args);
    }

    return 0;
}

int ArgParser_Version(int argc, char **argv, size_t command_idx)
{
    (void)argc;
    (void)argv;
    (void)command_idx;

    printf("Version: %s\n", SOFTWARE_VERSION);
    return 0;
}

int ArgParser_Generate(int argc, char **argv, size_t command_idx)
{
    if (argc < 3)
    {
        printf("Invalid number of arguments! Expected \"%s %s\"\n", argv[0], commands[command_idx].args);
        return 1;
    }

    int board_size = atoi(argv[2]);

    if (board_size < QUEENS_MIN_BOARD_SIZE || board_size > QUEENS_MAX_BOARD_SIZE)
    {
        printf("Invalid board size! Expected board size between %d and %d\n", QUEENS_MIN_BOARD_SIZE, QUEENS_MAX_BOARD_SIZE);
        return 1;
    }

    QueensPermutations_Result_t all_permutations = QueensPermutations_GetAll((QueensPermutation_BoardSize_t)board_size);
    QueensBoard_Board_t board = {0};
    bool ret = QueensBoard_Create(&board, (QueensBoard_Size_t)board_size);
    if (ret == false)
    {
        debug_print("Error allocating board!\n");
        return 1;
    }

    int n = 0;

    do
    {
        QueensBoardGen_Result_t ret = QueensBoardGen_Generate(&board, NULL);
        if (ret != QUEENS_BOARDGEN_SUCCESS)
        {
            debug_print("Error generating board!\n");
            return 1;
        }
        n++;
    } while (QueensBoardGen_ValidateOnlyOneSolution(&board, &all_permutations) == false);

    debug_print("\n");
    QueensBoard_PrintBoard(&board);

    debug_print("Number of iterations: %d\n", n);

    return 0;
}

int ArgParser_GenerateAndSolve(int argc, char **argv, size_t command_idx)
{
    if (argc < 3)
    {
        debug_print("Invalid number of arguments! Expected \"%s %s\"\n", argv[0], commands[command_idx].args);
        return 1;
    }

    int board_size = atoi(argv[2]);

    if (board_size < QUEENS_MIN_BOARD_SIZE || board_size > QUEENS_MAX_BOARD_SIZE)
    {
        debug_print("Invalid board size! Expected board size between %d and %d\n", QUEENS_MIN_BOARD_SIZE, QUEENS_MAX_BOARD_SIZE);
        return 1;
    }

    QueensPermutations_Result_t all_permutations = QueensPermutations_GetAll((QueensPermutation_BoardSize_t)board_size);
    QueensBoard_Board_t board = {0};
    bool ret = QueensBoard_Create(&board, (QueensBoard_Size_t)board_size);
    if (ret == false)
    {
        debug_print("Error allocating board!\n");
        return 1;
    }

    int n = 0;

    do
    {
        QueensBoardGen_Result_t ret = QueensBoardGen_Generate(&board, NULL);
        if (ret != QUEENS_BOARDGEN_SUCCESS)
        {
            debug_print("Error generating board!\n");
            return 1;
        }
        n++;
    } while (QueensBoardGen_ValidateOnlyOneSolution(&board, &all_permutations) == false);

    debug_print("\n");
    QueensBoard_PrintBoard(&board);

    debug_print("Number of iterations: %d\n", n);

    QueensSolver_Strategy_t strategy = QUEENS_SOLVER_FAILED;

    bool was_ngroups_strategy = false;
    bool was_forcing_strategy = false;
    uint16 iterations = 0u;

    do
    {
        iterations++;
        strategy = QueensSolver_IncrementalSolve(&board);
        if (strategy == QUEENS_SOLVER_STRATEGY_N_COLOR_GROUPS_OCCUPYING_N_ROWS_OR_COLUMNS)
        {
            was_ngroups_strategy = true;
        }
        else if (strategy == QUEENS_SOLVER_STRATEGY_QUEEN_PLACEMENT_LEADS_TO_INVALID_FORCING_SEQUENCE)
        {
            was_forcing_strategy = true;
        }
        debug_print("\n\nStrategy: %s\n", QueensSolver_GetStrategyName(strategy));
        QueensBoard_PrintBoard(&board);
        QueensBoard_PrintBoardAsString(&board);
    }
    while ((strategy != QUEENS_SOLVER_SOLVED) &&
           (strategy != QUEENS_SOLVER_FAILED));

    if (was_ngroups_strategy == true)
    {
        debug_print("NGroups strategy was used!\n");
    }

    if (was_forcing_strategy == true)
    {
        debug_print("Forcing strategy was used!\n");
    }

    debug_print("iterations: %u\n", iterations);


    return 0;
}

int ArgParser_SolveStep(int argc, char **argv, size_t command_idx)
{
    if (argc < 3)
    {
        printf("Invalid number of arguments! Expected \"%s %s\"\n", argv[0], commands[command_idx].args);
        return 1;
    }

    QueensBoard_Board_t board = {0};
    bool ret = QueensBoard_ParseFromString(argv[2], &board);
    if (ret == false)
    {
        printf("Error parsing board from string!\n");
        return 1;
    }

    QueensBoard_PrintBoard(&board);

    QueensSolver_Strategy_t strategy = QueensSolver_IncrementalSolve(&board);
    debug_print("\n\nStrategy: %s\n", QueensSolver_GetStrategyName(strategy));
    QueensBoard_PrintBoard(&board);
    QueensBoard_PrintBoardAsString(&board);

    return 0;
}

int ArgParser_PrintFromString(int argc, char **argv, size_t command_idx)
{
    if (argc < 3)
    {
        printf("Invalid number of arguments! Expected \"%s %s\"\n", argv[0], commands[command_idx].args);
        return 1;
    }

    QueensBoard_Board_t board = {0};
    bool ret = QueensBoard_ParseFromString(argv[2], &board);
    if (ret == false)
    {
        debug_print("Error parsing board from string!\n");
        return 1;
    }

    QueensBoard_PrintBoard(&board);

    return 0;
}
