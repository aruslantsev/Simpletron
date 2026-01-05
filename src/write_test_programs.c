#include <assert.h>
#include <stdio.h>
#include "simpletron.h"

void write_data(void) {
    const word_t header = HEADER;
    const word_t instructions[] = {READ, READ, LOAD, ADD, STORE, WRITE, HALT};
    const word_t operands[] = {0xa, 0xb, 0xa, 0xb, 0xc, 0xc, 0x0};
    assert(sizeof(instructions) / sizeof(word_t) == sizeof(operands) / sizeof(word_t));
    word_t instruction;
    char characrter_instruction[WORD_BITS / 4 + 2];  // 2 for \n\0

    FILE *binfile = fopen("add_numbers.sm", "wb");
    FILE *textfile = fopen("add_numbers.sml", "w");

    /* binary file starts with header */
    fwrite(&header, sizeof(word_t), 1, binfile);

    for (int i = 0; i < sizeof(instructions) / sizeof(word_t); i++) {
        instruction = (word_t) (instructions[i] << OPERAND_BITS | operands[i]);
        fwrite(&instruction, sizeof(word_t), 1, binfile);
        sprintf(characrter_instruction, "%*X\n", WORD_BITS / 4, (word_t) instruction);
        fprintf(textfile, "%s", characrter_instruction);
    }
    fclose(binfile);
    fclose(textfile);
}


int main() {
    write_data();
    return 0;
}