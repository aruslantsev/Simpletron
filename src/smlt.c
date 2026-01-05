#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "translator.h"
#include "simpletron.h"


#define DEBUG

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
            printf("%s", buffer);
            /* Remove trailing newline symbols before tokenization */
            buffer[strcspn(buffer, "\r\n")] = '\0';
            strip(buffer, buffer);
            printf("%s\n", buffer);
            /* Skip empty string */
            if (strlen(buffer) == 0) continue;
            parse_line(&program, buffer, line_number);
        }
    }

    /* Fill missing pointers */
#ifdef DEBUG
    puts("Resolving remaining references");
#endif
    const struct MissingRefListEntry *missing_ref_ptr = program.missing_ref_list;
    while (missing_ref_ptr != NULL) {
        identifier.value = missing_ref_ptr->label;
        address = search_entry(&program, identifier, LINE);
        if (address == -1) {
            printf("Unresolved label %d\n", missing_ref_ptr->label);
            exit(1);
        }
#ifdef DEBUG
        printf(
            "Instruction at address %0X references line %d. Real address: %ld\n",
            (word_t) missing_ref_ptr->address, missing_ref_ptr->label, address
        );
#endif
        program.memory[missing_ref_ptr->address] |= address;
        missing_ref_ptr = missing_ref_ptr->next;
    }
    fclose(program_file);

    /* Write program */
    puts("Writing program");
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
