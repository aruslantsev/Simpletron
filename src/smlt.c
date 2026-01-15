#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "translator.h"
#include "simpletron.h"


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
    word_t address;

    init_program(&program);

    while (!feof(program_file)) {
        if (fgets(buffer, BUFFER_SIZE, program_file) != NULL) {
            line_number++;

            /* Remove trailing newline symbols before tokenization */
            buffer[strcspn(buffer, "\r\n")] = '\0';
            strip(buffer, buffer);

            /* Skip empty string */
            if (strlen(buffer) == 0) continue;
            if (!parse_line(&program, buffer, line_number)) {
                printf("Error at line %d\n", line_number);
                printf("%s\n", buffer);
                fclose(program_file);
                exit(EXIT_FAILURE);
            }
        }
    }
    fclose(program_file);

    /* Fill missing pointers */
    for (
        size_t missing_ref_list_ptr = 0;
        missing_ref_list_ptr < program.missing_ref_list_size; 
        missing_ref_list_ptr++
    ) {
        identifier.value = program.missing_ref_list[missing_ref_list_ptr].label;
        address = search_entry(&program, identifier, LINE);
        if (address == OBJ_NOT_FOUND) {
            printf("Unresolved label %d\n", program.missing_ref_list[missing_ref_list_ptr].label);
            exit(1);
        }
        program.memory[program.missing_ref_list[missing_ref_list_ptr].address] |= address;
    }

    /* Fill stack offsets */
    for (
        size_t stack_offsets_ptr = 0;
        stack_offsets_ptr < program.stack_offset_list_size; 
        stack_offsets_ptr++
    ) {
        program.memory[program.stack_offset_list[stack_offsets_ptr].address] |= (
            program.constants_ptr - program.stack_offset_list[stack_offsets_ptr].offset
        );
    }

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
