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

    /* Fill stack offsets */
    for (
        size_t stack_offsets_ptr = 0;
        stack_offsets_ptr < program.stack_offset_list_size; 
        stack_offsets_ptr++
    ) {

#ifdef DEBUG
        printf(
            "Instruction at address %0X has stack offset %d. Real address: %ld (%0x)\n",
            (word_t) program.stack_offset_list[stack_offsets_ptr].address, 
            program.stack_offset_list[stack_offsets_ptr].offset, 
            program.constants_ptr - program.stack_offset_list[stack_offsets_ptr].offset, 
            (uword_t) (program.constants_ptr - program.stack_offset_list[stack_offsets_ptr].offset)
        );
#endif
        program.memory[program.stack_offset_list[stack_offsets_ptr].address] |= (
            program.constants_ptr - program.stack_offset_list[stack_offsets_ptr].offset
        );
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
