#pragma once
#include <stddef.h>

#include "simpletron.h"


#define IDENTIFIER_SIZE     8
#define BUFFER_SIZE         255
#define NUM_CONSTANTS       26

enum EntryType {CONST = 'c', LINE = 'l', VAR = 'v'};

union Identifier {
    int value;
    char name[IDENTIFIER_SIZE];
};

struct LookupListEntry {
    union Identifier            identifier;
    enum EntryType              type;
    size_t                      address;
    struct LookupListEntry      *next;
};

struct MissingRefListEntry {
    int                         label;
    size_t                      address;
    struct MissingRefListEntry  *next;
};

struct Program {
    struct LookupListEntry      *lookup_list;
    struct MissingRefListEntry  *missing_ref_list;
    word_t                      memory[MEMORY_SIZE];
    size_t                      instruction_ptr;
    size_t                      constants_ptr;
    size_t                      stack_ptr;
};


size_t search_entry(struct Program *, const union Identifier, const enum EntryType);
size_t add_entry(struct Program *, const union Identifier, const enum EntryType);
size_t search_or_add_entry(struct Program *, const union Identifier, const enum EntryType);
void remember_missing_reference(struct Program *, const int);
bool check_identifier(char []);
bool check_integer(char []);
void init_program(struct Program *);
void strip(char [], char []);
void parse_line(struct Program *, char [], const int);
void parse_input(struct Program *, char [], const int);
void parse_print(struct Program *, char [], const int);
void parse_goto(struct Program *, char [], const int);
void parse_let(struct Program *, char [], const int);
void parse_if(struct Program *, char [], const int);



bool check_expression(const char []);
bool evaluate_expression(char [], struct Program *);

enum Comparison {GE, GT, LE, LT, EQ, NE};
