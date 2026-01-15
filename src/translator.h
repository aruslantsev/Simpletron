#pragma once
#include <stddef.h>
#include "simpletron.h"


#define IDENTIFIER_SIZE     8
#define BUFFER_SIZE         255
#define OBJ_NOT_FOUND       ((word_t) -1)

enum EntryType {CONST = 'c', LINE = 'l', VAR = 'v'};

union Identifier {
    int value;
    char name[IDENTIFIER_SIZE];
};

struct LookupListEntry {
    union Identifier            identifier;
    enum EntryType              type;
    size_t                      address;
};

struct MissingRefListEntry {
    int                         label;
    size_t                      address;
};

struct ForEntry {
    word_t                      cycle_begin_address;
    word_t                      var_address;
    word_t                      to_address;
    word_t                      step_address;
};


struct StackOffsetEntry {
    word_t                      address;
    word_t                      offset;
};


struct Program {
    struct LookupListEntry      lookup_list[MEMORY_SIZE];
    struct MissingRefListEntry  missing_ref_list[MEMORY_SIZE];
    struct ForEntry             for_stack[MEMORY_SIZE];
    struct StackOffsetEntry     stack_offset_list[MEMORY_SIZE];
    word_t                      memory[MEMORY_SIZE];
    word_t                      instruction_ptr;
    word_t                      constants_ptr;
    size_t                      stack_ptr;
    size_t                      lookup_list_size;
    size_t                      missing_ref_list_size;
    size_t                      for_ptr;
    size_t                      stack_offset_list_size;
};


word_t search_entry(struct Program *, const union Identifier, const enum EntryType);
word_t add_entry(struct Program *, const union Identifier, const enum EntryType);
word_t search_or_add_entry(struct Program *, const union Identifier, const enum EntryType);
void remember_missing_reference(struct Program *, const int);
bool check_identifier(char []);
bool check_integer(char []);
void init_program(struct Program *);
void strip(char [], const char []);
bool parse_line(struct Program *, char [], const int);
bool parse_input(struct Program *, char [], const int);
bool parse_print(struct Program *, char [], const int);
bool parse_goto(struct Program *, char [], const int);
bool parse_let(struct Program *, char [], const int);
bool parse_if(struct Program *, char [], const int);
bool parse_for(struct Program *, char [], const int);
bool parse_for_end(struct Program *, char [], const int);

bool check_expression(const char []);
bool evaluate_expression(char [], struct Program *);

enum Comparison {GE, GT, LE, LT, EQ, NE};
