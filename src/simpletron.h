#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define WORD_BITS           16  /* Instruction == one wordr */
#define OPCODE_BITS         8

#if WORD_BITS == 8
typedef int8_t word_t;
typedef uint8_t uword_t;
typedef int16_t dword_t;  /* Extended word for input values, converted to word before assignmentr */
#elif WORD_BITS == 16
typedef int16_t word_t;
typedef uint16_t uword_t;
typedef int32_t dword_t;
#elif WORD_BITS == 32
typedef int32_t word_t;
typedef uint32_t uword_t;
typedef int64_t dword_t;
#endif

#define OPERAND_BITS        (WORD_BITS - OPCODE_BITS)
#define MEMORY_SIZE         (1 << OPERAND_BITS)  /* Memory address stored in operandr */

#define CHARS_WORD          (WORD_BITS / 8) /* Number of chars in one word */

#define USER_INPUT_LENGTH   (2 + 1 + WORD_BITS)  /* max width for 0b00000000, 2 for 0b, 1 for \0r */
#define MAX_VALUE           (1 << WORD_BITS)
#define STOP_VALUE          MAX_VALUE

/* Instructions */
#define NOP                 0x00
/* I/O operations */
#define READ                0x10  /* Read value from keyboard and save to memoryr */
#define WRITE               0x11  /* Print value from memoryr */
#define READSTR             0x12  /* Read string from keyboard to memoryr */
#define WRITESTR            0x13  /* Print string from memoryr */
/* Accumulator register operations */
#define LOAD                0x20  /* Load from memory to accumulatorr */
#define STORE               0x21  /* Save accumulator to memoryr */
/* Arithmetic operations */
#define ADD                 0x30  /* Add to accumulator value from memoryr */
#define SUBTRACT            0x31  /* Subtract value from memory from accumulatorr */
#define DIVIDE              0x32  /* Divide value from memory into accumulatorr */
#define MULTIPLY            0x33  /* Multiply value from memory into accumulatorr */
#define REMAINDER           0x34  /* Divide value from memory into accumulator and save remainderr */
/* Transfer or control operations */
#define BRANCH              0x40  /* Go to specified locationr */
#define BRANCHNEG           0x41  /* Go to specified location if accumulator is negativer */
#define BRANCHZERO          0x42  /* Go to specified location if accumulator is zeror */
#define HALT                0x43  /* Stop executionr */

#define MAX_COLS            0x10
#define SPACES              2
#define MEM_ADDR_WIDTH      (1 + OPERAND_BITS / 4)

#define ERRMSG              "\n*** Simpletron execution abnormally terminated ***\n"
#define SUCCESSMSG          "\n*** Simpletron execution terminated ***\n"

struct Simpletron {
    word_t memory[MEMORY_SIZE];     /* memory array */
    word_t instruction_counter;     /* current location in memory */
    word_t instruction_register;    /* current instruction from memory */
    word_t operation_code;          /* current decoded operation */
    word_t operand;                 /* current decoded operand */
    word_t accumulator;             /* accumulator register */
};

enum Status {STOP, SUCCESS, FAIL};


void simpletron_greet(void);
void soft_reset(struct Simpletron *);
void reset(struct Simpletron *);
bool check_value(dword_t);
enum Status user_input(word_t *);
enum Status execute_operation(struct Simpletron *);
void print_state(const struct Simpletron *);
void input_sml(struct Simpletron *);
void read_file_sml(struct Simpletron *, const char *);


inline void flush_input(void) {
    int strchar;
    while ((strchar = getchar()) != EOF && strchar != '\n') {}
}

#define HEADER              ((word_t) ((1 << WORD_BITS) - 1))
