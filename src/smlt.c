#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "translator.h"
#include "simpletron.h"


// #define DEBUG

int main(const int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[0], "--help") == 0 || strcmp(argv[0], "-h") == 0) {
        puts("Usage:");
        printf("\t%s\tFILENAME.bas OUTFILE.sml\n", argv[0]);
        return 0;
    }

    FILE *program_file = fopen(argv[1], "r");
    if (program_file == NULL) {
        puts("Error opening input file");
        exit(1);
    }

    struct Program program;
    char buffer[BUFFER_SIZE];
    int line_number = 0;
    union Identifier identifier;
    size_t address;

    init_program(&program);

    while (!feof(program_file)) {
        if (fgets(buffer, BUFFER_SIZE, program_file) != NULL) {
            line_number++;
#ifdef DEBUG
            printf("%s", buffer);
#endif
            /* Remove trailing newline symbols before tokenization */
            buffer[strcspn(buffer, "\r\n")] = '\0';
            strip(buffer, buffer);
#ifdef DEBUG
            printf("%s\n", buffer);
#endif
            /* Skip empty string */
            if (strlen(buffer) == 0) continue;
            parse_line(&program, buffer, line_number);
        }
    }

    /* Fill missing pointers */
#ifdef DEBUG
    puts("Resolving remaining references");
#endif
    for (
        size_t missing_ref_list_ptr = 0;
        missing_ref_list_ptr < program.missing_ref_list_size; 
        missing_ref_list_ptr++
    ) {
        identifier.value = program.missing_ref_list[missing_ref_list_ptr].label;
        address = search_entry(&program, identifier, LINE);
        if (address == -1) {
            printf("Unresolved label %d\n", program.missing_ref_list[missing_ref_list_ptr].label);
            exit(1);
        }
#ifdef DEBUG
        printf(
            "Instruction at address %0X references line %d. Real address: %ld (%0x)\n",
            (word_t) program.missing_ref_list[missing_ref_list_ptr].address, 
            program.missing_ref_list[missing_ref_list_ptr].label, 
            address, 
            (uword_t) address
        );
#endif
        program.memory[program.missing_ref_list[missing_ref_list_ptr].address] |= address;
    }
    fclose(program_file);

#ifdef DEBUG
    puts("Writing program");
#endif
    FILE *sml_file = fopen(argv[2], "w");
    char char_instruction[WORD_BITS / 4 + 2];
    for (int instructionPtr = 0; instructionPtr < MEMORY_SIZE; instructionPtr++) {
        sprintf(
            char_instruction, "%*X\n", WORD_BITS / 4, (uword_t) program.memory[instructionPtr]
        );
        fprintf(sml_file, "%s", char_instruction);
    }
    fclose(sml_file);
    return 0;
}
