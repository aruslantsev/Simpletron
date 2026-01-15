#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "simpletron.h"


void simpletron_greet(void) {
    printf(
        "*** Welcome to Simpletron! ***\n"
        "*** Please enter your program one instruction ***\n"
        "*** (or data word) at a time. I will type the ***\n"
        "*** location number and a question mark (?). ***\n"
        "*** You then type the hexadecimal word for that  ***\n"
        "*** location. Type the sentinel %X to stop entering ***\n"
        "*** your program. ***\n\n",
        STOP_VALUE
    );
}


void soft_reset(struct Simpletron *simpletron) {
    simpletron->accumulator = 0;
    simpletron->instruction_counter = 0;
    simpletron->instruction_register = 0;
    simpletron->operation_code = 0;
    simpletron->operand = 0;
}


void reset(struct Simpletron *simpletron) {
    soft_reset(simpletron);
    for (size_t counter = 0; counter < MEMORY_SIZE; simpletron->memory[counter++] = 0);
}


bool check_value(const dword_t value) {
    if (labs(value) < MAX_VALUE)
        return true;
    return false;
}


enum Status user_input(word_t *value) {
    char s[USER_INPUT_LENGTH];
    fgets(s, USER_INPUT_LENGTH, stdin);
    const dword_t parsed_input = (word_t) strtol(s, NULL, 0);
    if (check_value(parsed_input)) {
        *value = (word_t) parsed_input;
        return SUCCESS;
    }
    return FAIL;
}


enum Status execute_operation(struct Simpletron *simpletron) {
    if (simpletron->instruction_counter < 0 || simpletron->instruction_counter >= MEMORY_SIZE) {
        printf("*** instructionCounter is not in range 0..%d ***\n", MEMORY_SIZE);
        puts(ERRMSG);
        return FAIL;
    }

    /* Decode instruction */
    simpletron->instruction_register = simpletron->memory[simpletron->instruction_counter++];
    if (simpletron->instruction_register < 0) {
        printf(
                "*** Invalid instruction %d at %d ***\n",
                simpletron->instruction_register, simpletron->instruction_counter - 1
            );
        return FAIL;
    }
    simpletron->operation_code = (uword_t) simpletron->instruction_register >> OPERAND_BITS;
    simpletron->operand = (uword_t) simpletron->instruction_register & ((1 << OPERAND_BITS) - 1);

    /* Execute instruction */
    size_t memptr = simpletron->operand;
    int strchar;
    size_t word_idx = 0;

    switch (simpletron->operation_code) {
        case NOP:
            break;
        case READ:
            printf("%s", "<- ");
            if (user_input(&simpletron->memory[simpletron->operand]) != SUCCESS) {
                printf(
                    "*** Invalid input. Should be in decimal range %d..%d ***\n",
                    -MAX_VALUE + 1, MAX_VALUE - 1
                );
                return FAIL;
            }
            break;
        case WRITE:
            printf("-> %+0*d\n", WORD_BITS / 4 + 1, simpletron->memory[simpletron->operand]);
            break;
        case READSTR:
            printf("%s", "<- ");
            word_idx = 0;
            simpletron->memory[memptr] = 0;
            while ((strchar = getchar()) != '\n') {
                simpletron->memory[memptr] |= ((uint8_t) strchar) << (8 * word_idx++);
                if (word_idx >= CHARS_WORD) {
                    word_idx = 0;
                    memptr++;
                    simpletron->memory[memptr] = 0;
                }
            }
            simpletron->memory[memptr] &= ((uint8_t) 0) << word_idx; /* Force terminate string */
            break;
        case WRITESTR:
            printf("%s", "-> ");
            word_idx = 0;
            while ((strchar = (int8_t) (simpletron->memory[memptr] >> (8 * word_idx++))) != 0) {
                putchar(strchar);
                if (word_idx >= CHARS_WORD) {
                    word_idx = 0;
                    memptr++;
                }
            }
            putchar('\n');
            break;
        case LOAD:
            simpletron->accumulator = simpletron->memory[simpletron->operand];
            break;
        case STORE:
            simpletron->memory[simpletron->operand] = simpletron->accumulator;
            break;
        case ADD:
            simpletron->accumulator += simpletron->memory[simpletron->operand];
            break;
        case SUBTRACT:
            simpletron->accumulator = (
                simpletron->memory[simpletron->operand] - simpletron->accumulator
            );
            break;
        case DIVIDE:
            if (simpletron->accumulator != 0) {
                simpletron->accumulator = (
                    simpletron->memory[simpletron->operand] / simpletron->accumulator
                );
            } else {
                printf(
                    "*** Attempt to divide by zero at %d ***\n",
                    simpletron->instruction_counter - 1
                );
                puts(ERRMSG);
                return FAIL;
            }
            break;
        case MULTIPLY:
            simpletron->accumulator *= simpletron->memory[simpletron->operand];
            break;
        case REMAINDER:
            if (simpletron->accumulator != 0) {
                simpletron->accumulator = (
                    simpletron->memory[simpletron->operand] % simpletron->accumulator
                );
            } else {
                printf(
                    "*** Attempt to divide by zero at %d ***\n",
                    simpletron->instruction_counter - 1
                );
                puts(ERRMSG);
                return FAIL;
            }
            break;
        case POWER:
            simpletron->accumulator = (word_t) pow(
                simpletron->memory[simpletron->operand], simpletron->accumulator
            );
            break;
        case BRANCH:
            simpletron->instruction_counter = simpletron->operand;
            break;
        case BRANCHNEG:
            if (simpletron->accumulator < 0) {
                simpletron->instruction_counter = simpletron->operand;
            }
            break;
        case BRANCHZERO:
            if (simpletron->accumulator == 0) {
                simpletron->instruction_counter = simpletron->operand;
            }
            break;
        case HALT:
            puts(SUCCESSMSG);
            return STOP;
        default:
            printf(
                "*** Invalid instruction %X at %X ***\n",
                simpletron->instruction_register, simpletron->instruction_counter - 1
            );
            puts(ERRMSG);
            return FAIL;
    }
    if (check_value(simpletron->accumulator))
        return SUCCESS;
    printf(
    "*** Accumulator value %d is not in range %d..%d ***\n",
        simpletron->accumulator, MAX_VALUE, -MAX_VALUE
    );
    puts(ERRMSG);
    return FAIL;
}


void print_state(const struct Simpletron *simpletron) {
    printf("accumulator:\t\t%0*X\n", WORD_BITS / 4, (uword_t) simpletron->accumulator);
    printf(
        "instructionCounter:\t%*X\n", WORD_BITS / 4, (uword_t) simpletron->instruction_counter
    );
    printf(
        "instructionRegister:\t%0*X\n", WORD_BITS / 4, (uword_t) simpletron->instruction_register
    );
    printf("operationCode:\t\t%*X\n", WORD_BITS / 4, (uword_t) simpletron->operation_code);
    printf("operand:\t\t%*X\n", WORD_BITS / 4, (uword_t) simpletron->operand);
    puts("\nMEMORY:");
    printf("%*s", MEM_ADDR_WIDTH, "");
    for (size_t counter = 0; counter < MAX_COLS; counter++)
        printf("%*lX", SPACES + WORD_BITS / 4, counter);
    puts("");

    for (size_t counter = 0; counter < MEMORY_SIZE; counter++) {
        if (counter % MAX_COLS == 0) {
            if (counter > 0)
                puts("");
            printf("%*lX", MEM_ADDR_WIDTH, counter / MAX_COLS);
        }
        printf("%*s%0*X", SPACES, "", WORD_BITS / 4, (uword_t) simpletron->memory[counter]);
    }
    puts("\n");
}


void input_sml(struct Simpletron *simpletron) {
    dword_t input;
    char s[USER_INPUT_LENGTH];

    reset(simpletron);
    simpletron_greet();
    do {
        printf("%0*x ? ", OPERAND_BITS / 4, simpletron->instruction_counter);
        fgets(s, USER_INPUT_LENGTH, stdin);
        input = strtol(s, NULL, 16);

        if (check_value(input)) {
            simpletron->memory[simpletron->instruction_counter++] = (word_t) input;
        } else if (input != STOP_VALUE) {
            puts("Invalid input");
        }
    } while (input != STOP_VALUE);
    soft_reset(simpletron);
}


void read_file_sml(struct Simpletron *simpletron, const char *filename) {
    word_t header;
    size_t result = 0;
    reset(simpletron);

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Error opening file '%s'\n\n",  filename);
        exit(1);
    }
    result = fread(&header, sizeof(word_t), 1, file);
    if (result == 0) {
        printf("Error reading file '%s'\n\n",  filename);
        exit(1);
    }
    if (header == HEADER) { /* binary file */
        puts("Got binary Simpletron memory state");
        while (!feof(file)) {
            result = fread(
                &simpletron->memory[simpletron->instruction_counter++],
                sizeof(word_t),
                1,
                file
            );
            if (result == 0 && !feof(file)) puts("Error reading file");
        }
    } else {
        fclose(file);
        puts("Got text Simpletron Machine Language file");
        file = fopen(filename, "r");
        dword_t input;
        char s[USER_INPUT_LENGTH];
        while (!feof(file)) {
            if (fgets(s, USER_INPUT_LENGTH, file) != NULL) {
                input = strtol(s, NULL, 16);
                if (check_value(input)) {
                    simpletron->memory[simpletron->instruction_counter++] = (word_t) input;
                } else printf("Invalid input: %s\n", s);
            }
        }
    }
    fclose(file);
    soft_reset(simpletron);
}
