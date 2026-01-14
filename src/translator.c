#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simpletron.h"
#include "translator.h"
#include "evaluate.h"


// #define DEBUG_LOOKUP_LIST
// #define DEBUG_MISSING_REF_LIST
// #define DEBUG_PARSE
// #define DEBUG_EXPRESSION


void init_program(struct Program *program) {
    for (size_t ptr = 0; ptr < MEMORY_SIZE; ptr++) {
        program->memory[ptr] = 0;
    }
    program->instruction_ptr = 0;
    program->constants_ptr = MEMORY_SIZE - 1;
    program->stack_ptr = program->constants_ptr - NUM_CONSTANTS;
    program->lookup_list_size = 0;
    program->missing_ref_list_size = 0;
}


size_t search_entry(
    struct Program *program,
    const union Identifier identifier,
    const enum EntryType type
) {
#ifdef DEBUG_LOOKUP_LIST
    printf("Searching for %c", type);
    if (type == VAR) {
        printf(" %s\n",identifier.name);
    } else {
        printf(" %d\n", identifier.value);
    }
#endif
    for (
        size_t lookup_list_ptr = 0; lookup_list_ptr < program->lookup_list_size; lookup_list_ptr++
    ) {
#ifdef DEBUG_LOOKUP_LIST
        printf("Scanning object: %c", program->lookup_list[lookup_list_ptr].type);
        if (program->lookup_list[lookup_list_ptr].type == VAR) {
            printf(" %s", program->lookup_list[lookup_list_ptr].identifier.name);
        } else {
            printf(" %d", program->lookup_list[lookup_list_ptr].identifier.value);
        }
        printf(
            ", address %ld (%0X)\n",
            program->lookup_list[lookup_list_ptr].address, 
            (uword_t) program->lookup_list[lookup_list_ptr].address
        );
#endif
        if (
            (
                type == VAR
                && type == program->lookup_list[lookup_list_ptr].type
                && strcmp(
                    program->lookup_list[lookup_list_ptr].identifier.name, identifier.name
                ) == 0
            )
            || (
                (type == CONST || type == LINE)
                && type == program->lookup_list[lookup_list_ptr].type
                && program->lookup_list[lookup_list_ptr].identifier.value == identifier.value
            )
        ) {
#ifdef DEBUG_LOOKUP_LIST
            printf(
                "Found, address: %ld (%0X)\n",
                program->lookup_list[lookup_list_ptr].address, 
                (uword_t) program->lookup_list[lookup_list_ptr].address
            );
#endif
            return program->lookup_list[lookup_list_ptr].address;
        }
    }
#ifdef DEBUG_LOOKUP_LIST
    puts("Not found");
#endif
    return -1;
}


size_t add_entry(
    struct Program *program,
    const union Identifier identifier,
    const enum EntryType type
) {
#ifdef DEBUG_LOOKUP_LIST
    printf("Adding %c", type);
    if (type == VAR) {
        printf(" %s\n",identifier.name);
    } else {
        printf(" %d\n", identifier.value);
    }
#endif
    program->lookup_list[program->lookup_list_size].identifier = identifier;
    program->lookup_list[program->lookup_list_size].type = type;

    if (type == LINE) {
        program->lookup_list[program->lookup_list_size].address = program->instruction_ptr;
    } else {
        program->lookup_list[program->lookup_list_size].address = program->constants_ptr;
        if (type == CONST) {
            program->memory[program->constants_ptr] = identifier.value;
#ifdef DEBUG_LOOKUP_LIST
            printf(
                "Writing constant value %d (%0X) at %ld (%0X)\n",
                identifier.value, (word_t) identifier.value,
                program->constants_ptr, (uword_t) program->constants_ptr
            );
#endif
        }
        program->constants_ptr--;
    }
#ifdef DEBUG_LOOKUP_LIST
    printf("Added object: %c", program->lookup_list[program->lookup_list_size].type);
    if (program->lookup_list[program->lookup_list_size].type == VAR) {
        printf(" %s", program->lookup_list[program->lookup_list_size].identifier.name);
    } else {
        printf(" %d", program->lookup_list[program->lookup_list_size].identifier.value);
    }
    printf(
        ", address %ld (%0X)\n", 
        program->lookup_list[program->lookup_list_size].address, 
        (uword_t) program->lookup_list[program->lookup_list_size].address
    );
#endif
    program->lookup_list_size++;
    return program->lookup_list[program->lookup_list_size - 1].address;
}


size_t search_or_add_entry(
    struct Program *program,
    const union Identifier identifier,
    const enum EntryType type
) {
#ifdef DEBUG_LOOKUP_LIST
    printf("Searching and adding %c", type);
    if (type == VAR) {
        printf(" %s\n",identifier.name);
    } else {
        printf(" %d\n", identifier.value);
    }
#endif
    const size_t address = search_entry(program, identifier, type);
    if (address != -1) {
        return address;
    }
    return add_entry(program, identifier, type);
}


void remember_missing_reference(struct Program *program, const int identifier) {
#ifdef DEBUG_MISSING_REF_LIST
    printf(
        "Adding missing reference to row number %d for address %ld (%0X)\n",
        identifier, program->instruction_ptr, (uword_t) program->instruction_ptr
    );
#endif
    program->missing_ref_list[program->missing_ref_list_size++] = (struct MissingRefListEntry) {
        .label=identifier, .address=program->instruction_ptr
    };
#ifdef DEBUG_MISSING_REF_LIST
    puts("Done.");
#endif
}


bool check_identifier(char identifier[]) {
    if (strlen(identifier) == 0) return false;
    if (!isalpha(identifier[0]) && identifier[0] != '_') return false;
    for (char *ptr = identifier; *ptr != '\0'; ptr++) {
        if (!(isalnum(*ptr) || *ptr == '_')) {
            return false;
        }
    }
    return true;
}


bool check_integer(char value[]) {
    if (strlen(value) == 0) return false;
    const char *ptr = value;
    bool has_digits = false;
    if (*ptr == '-' || *ptr == '+') ptr++;
    for (; *ptr != '\0'; ptr++) {
        if (!isdigit(*ptr)) return false;
        has_digits = true;
    }
    return has_digits;
}


void strip(char s1[], char s2[]) {
    /* Remove leading and trailing spaces and replace duplicated spaces in s2,
     * save result in s1. s2 remains unmodified */
    char buffer[BUFFER_SIZE];
    strcpy(buffer, s2);
    int i = 0, j = 0;
    while (isspace(buffer[i])) i++;
    for (; buffer[i] != '\0'; i++) {
        if (isspace(buffer[i])) {
            if (j == 0 || !isspace(s1[j - 1]) ) {
                s1[j++] = ' ';
            }
        } else {
            s1[j++] = buffer[i];
        }
    }
    /* Null-terminate the modified string */
    s1[j] = '\0';
    /* Trim a single trailing space if it exists */
    if (j > 0 && isspace(s1[j - 1])) {
        s1[j - 1] = '\0';
    }
}


void parse_line(struct Program *program, char line[], const int line_number) {
    char buffer[BUFFER_SIZE];
    union Identifier identifier;
    strcpy(buffer, line);

    /* First token: line number */
    char *token = strtok(buffer, " ");
    if (!check_integer(token)) {
        printf("Wrong line number in line %d\n", line_number);
        printf("%s\n", line);
    }
    identifier.value = atoi(token);
    if (identifier.value < 0) {
        printf(
            "Line number %d should be non-negative in line %d\n",
            identifier.value, line_number
        );
        printf("%s\n", line);
        exit(1);
    }
    if (search_entry(program, identifier, LINE) != -1) {
        printf(
            "Error: duplicated line number '%d' in line %d\n",
            identifier.value, line_number
        );
        printf("%s\n", line);
        exit(1);
    }
    const size_t address = add_entry(program, identifier, LINE);
#ifdef DEBUG_PARSE
    printf(
        "Added entry for line %d in table, address %ld (%0X)\n",
        identifier.value, address, (uword_t) address
    );
#endif

    /* Second token: rem, let, input, print, goto, if ... goto, end */
    token = strtok(NULL, " ");
    if (token == NULL) return;  /* Empty line */
#ifdef DEBUG_PARSE
    printf("Got keyword '%s'\n", token);
#endif
    if (strcmp("rem", token) == 0) return;  /* Skip comments */
    if (strcmp("input", token) == 0) parse_input(program, line, line_number);
    else if (strcmp("print", token) == 0) parse_print(program, line, line_number);
    else if (strcmp("let", token) == 0) parse_let(program, line, line_number);
    else if (strcmp("goto", token) == 0) parse_goto(program, line, line_number);
    else if (strcmp("if", token) == 0) parse_if(program, line, line_number);
    else if (strcmp("end", token) == 0) {
        const word_t instruction = HALT << OPERAND_BITS;
        program->memory[program->instruction_ptr++] = instruction;
    } else {
        printf("Wrong token '%s' in line %d\n", token, line_number);
        printf("%s\n", line);
        exit(1);
    }
}


void parse_input(struct Program *program, char line[], const int line_number) {
    word_t instruction;
    char buffer[BUFFER_SIZE];
    union Identifier identifier;
    size_t address;

#ifdef DEBUG_PARSE
    puts("Parsing INPUT instruction");
#endif
    strcpy(buffer, line);
    strtok(buffer, " ");  /* Skip line number */
    strtok(NULL, " ");  /* Skip keyword */
    char *token = strtok(NULL, " ,");  /* Tokenize remaining string by space of comma */
    if (token == NULL) {
        printf("Missing variable name after INPUT keyword in line %d", line_number);
        printf("%s\n", line);
        exit(1);
    }
    while (token != NULL) {
        if (!check_identifier(token)) {
            printf("Wrong variable name %s in line %d", token, line_number);
            printf("%s\n", line);
            exit(1);
        }
        strncpy(identifier.name, token, IDENTIFIER_SIZE - 1);
#ifdef DEBUG_PARSE
        printf("Adding variable %s\n", identifier.name);
#endif
        address = search_or_add_entry(program, identifier, VAR);
        if (address == -1) {
            printf("Unknown error in line %d\n", line_number);
            printf("%s\n", line);
            exit(1);
        }
        instruction = READ << OPERAND_BITS | address;
#ifdef DEBUG_PARSE
        printf("Instruction %0x\n", instruction);
#endif
        program->memory[program->instruction_ptr++] = instruction;
        token = strtok(NULL, " ,");
    }
}


void parse_print(struct Program *program, char line[], const int line_number) {
    word_t instruction;
    char buffer[BUFFER_SIZE];
    union Identifier identifier;
    size_t address;

#ifdef DEBUG_PARSE
    puts("Parsing PRINT instruction");
#endif
    strcpy(buffer, line);
    strtok(buffer, " ");  /* Skip line number */
    strtok(NULL, " ");  /* Skip keyword */
    char *token = strtok(NULL, " ,");  /* Tokenize remaining string by space of comma */
    if (token == NULL) {
        printf("Missing variable name after PRINT keyword in line %d", line_number);
        printf("%s\n", line);
        exit(1);
    }
    while (token != NULL) {
        if (!check_identifier(token)) {
            printf("Wrong variable name %s in line %d", token, line_number);
            printf("%s\n", line);
            exit(1);
        }
        strncpy(identifier.name, token, IDENTIFIER_SIZE - 1);
#ifdef DEBUG_PARSE
        printf("Adding variable %s\n", identifier.name);
#endif
        address = search_or_add_entry(program, identifier, VAR);
        if (address == -1) {
            printf("Unknown error in line %d\n", line_number);
            printf("%s\n", line);
            exit(1);
        }
        instruction = WRITE << OPERAND_BITS | address;
#ifdef DEBUG_PARSE
        printf("Instruction %0x\n", instruction);
#endif
        program->memory[program->instruction_ptr++] = instruction;
        token = strtok(NULL, " ,");
    }
}


void parse_goto(struct Program *program, char line[], const int line_number) {
    word_t instruction;
    char buffer[BUFFER_SIZE];
    union Identifier identifier;
    size_t address;
    bool is_missing = false;

#ifdef DEBUG_PARSE
    puts("Parsing GOTO instruction");
#endif
    strcpy(buffer, line);
    strtok(buffer, " ");  /* Skip line number */
    strtok(NULL, " ");  /* Skip keyword */
    char *token = strtok(NULL, " ");
    if (token == NULL) {
        printf("Missing line number after GOTO keyword in line %d\n", line_number);
        printf("%s\n", line);
        exit(1);
    }
    if (!check_integer(token)) {
        printf("Bad line number in line %d\n", line_number);
        printf("%s\n", line);
        exit(1);
    }
    identifier.value = atoi(token);
    if (identifier.value < 0) {
        printf(
            "Line number %d should be non-negative in line %d\n",
            identifier.value, line_number
        );
        printf("%s\n", line);
        exit(1);
    }

    token = strtok(NULL, " ");
    if (token != NULL) {
        printf("Error: multiple line numbers after GOTO keyword in line %d\n", line_number);
        printf("%s\n", line);
        exit(1);
    }
    if ((address = search_entry(program, identifier, LINE)) == -1) {
        remember_missing_reference(program, identifier.value);
        address = 0;
        is_missing = true;
    }
#ifdef DEBUG_PARSE
    printf("GOTO line %d.", identifier.value);
    if (is_missing) {
        puts(" Reference is missing.");
    } else {
        printf(" Address: %ld\n", address);
    }
#endif
    instruction = BRANCH << OPERAND_BITS | address;
#ifdef DEBUG_PARSE
    printf("Instruction %0x\n", instruction);
#endif
    program->memory[program->instruction_ptr++] = instruction;
}


void parse_let(struct Program *program, char line[], const int line_number) {
    char buffer[BUFFER_SIZE];
    union Identifier identifier;

#ifdef DEBUG_PARSE
    puts("Parsing LET instruction");
#endif
    strcpy(buffer, line);
    char *buffer_start = buffer;
    strtok(buffer, " ");  /* Skip line number */
    strtok(NULL, " ");  /* Skip keyword */

    char *token = strtok(NULL, " =");
    if (!check_identifier(token)) {
        printf("Wrong variable name %s in line %d", token, line_number);
        printf("%s\n", line);
        exit(1);
    }
    if (token == NULL) {
        printf("Missing expression after LET keyword in line %d\n", line_number);
        printf("%s\n", line);
        exit(1);
    }
    strncpy(identifier.name, token, IDENTIFIER_SIZE - 1);
#ifdef DEBUG_PARSE
    printf("Adding variable %s\n", identifier.name);
#endif
    size_t address = search_or_add_entry(program, identifier, VAR);
    if (address == -1) {
        printf("Unknown error in line %d\n", line_number);
        printf("%s\n", line);
        exit(1);
    }
    size_t str_ptr = token - buffer_start + strlen(token);  /* End of variable name */
    /* Check space between variable name and '='. Should be only spaces */
    for (; line[str_ptr] != '='; str_ptr++) {
        /* End of line before assignment */
        if (line[str_ptr] == '\0') {
            printf("Unexpected end of line in line %d\n", line_number);
            printf("%s\n", line);
            exit(1);
        }
        /* Non-space symbol */
        if (line[str_ptr] != ' ') {
            printf("Wrong format or multiple assignment in line %d\n", line_number);
            printf("%s\n", line);
            exit(1);
        }
    }
    str_ptr++; /* At '=' now */
    /* Copy expression to evaluate */
    strcpy(buffer, &line[str_ptr]);
    strip(buffer, buffer);
#ifdef DEBUG_PARSE
    printf("Validating '%s'\n", buffer);
#endif
    if (!check_expression(buffer)) {
        printf("Invalid expression '%s' on line %d\n", buffer, line_number);
        printf("%s\n", line);
        exit(1);
    }
    if (!evaluate_expression(buffer, program)) {
        printf("Invalid expression '%s' in line %d\n", buffer, line_number);
        printf("%s\n", line);
        exit(1);
    }
    const word_t instruction = STORE << OPERAND_BITS | address;
    program->memory[program->instruction_ptr++] = instruction;
}


void parse_if(struct Program *program, char line[], const int line_number) {
    word_t instruction;
    char buffer[BUFFER_SIZE];
    union Identifier identifier;
    size_t address;

#ifdef DEBUG_PARSE
    puts("Parsing IF instruction");
#endif
    strcpy(buffer, line);
    const char *buffer_start = buffer;
    strtok(buffer, " ");  /* Skip line number */
    char *token = strtok(NULL, " ");  /* Skip keyword */

    /* Copy from start of left expression */
    strcpy(buffer, &line[token - buffer_start + strlen(token)]);
    strip(buffer, buffer);
#ifdef DEBUG_PARSE
    printf("Parsing condition: %s\n", buffer);
#endif
    char *condition_end = strstr(buffer, "goto");
    if (condition_end == NULL) {
        printf("Missing GOTO in IF statement in line %d\n", line_number);
        printf("%s\n", line);
        exit(1);
    }
    char expr[BUFFER_SIZE];
    strncpy(expr, buffer, condition_end - buffer);
    expr[condition_end - buffer] = '\0';
    condition_end += 4;  /* Skip GOTO keyword */
    token = strtok(condition_end, " ");
    if (token == NULL) {
        printf("Missing line number after IF..GOTO keyword in line %d\n", line_number);
        printf("%s\n", line);
        exit(1);
    }
    if (!check_integer(token)) {
        printf("Bad line number in line %d\n", line_number);
        printf("%s\n", line);
    }
    identifier.value = atoi(token);
    if (identifier.value < 0) {
        printf(
            "Line number %d should be non-negative in line %d\n",
            identifier.value, line_number
        );
        printf("%s\n", line);
        exit(1);
    }
    bool add_missing = false;
    if ((address = search_entry(program, identifier, LINE)) == -1) {
        add_missing = true;
        address = 0;
    }
#ifdef DEBUG_PARSE
    printf("Condition: '%s', goto row: '%d'\n", expr, identifier.value);
#endif
    token = strtok(NULL, " ");
    if (token != NULL) {
        printf(
            "Error: multiple line numbers after IF..GOTO keyword in line %d\n",
            line_number
        );
        printf("%s\n", line);
        exit(1);
    }

    char left_expression[BUFFER_SIZE];
    char right_expression[BUFFER_SIZE];
    char transformed_expression[BUFFER_SIZE];
    char *comparison_position;
    enum Comparison comparison;

    if ((comparison_position = strstr(expr, "<=")) != NULL) {
        comparison = LE;
    } else if ((comparison_position = strstr(expr, ">=")) != NULL) {
        comparison = GE;
    } else if ((comparison_position = strstr(expr, "<")) != NULL) {
        comparison = LT;
    } else if ((comparison_position = strstr(expr, ">")) != NULL) {
        comparison = GT;
    } else if ((comparison_position = strstr(expr, "==")) != NULL) {
        comparison = EQ;
    } else if ((comparison_position = strstr(expr, "!=")) != NULL) {
        comparison = NE;
    } else {
        printf("Can't find comparison in expression '%s' in line %d\n", expr, line_number);
        printf("%s\n", line);
        exit(1);
    }

    strncpy(left_expression, expr, comparison_position - expr);
    left_expression[comparison_position - expr] = '\0';
    strcpy(
        right_expression,
        comparison_position + ((comparison == LT || comparison == GT)? 1: 2)
    );

    switch (comparison) {
        case LE:
        case LT:
        case EQ:
        case NE:
            sprintf(
                transformed_expression, "(%s) - (%s)", left_expression, right_expression
            );
            break;
        case GE:
        case GT:
            sprintf(
                transformed_expression, "(%s) - (%s)", right_expression, left_expression
            );
            break;
        default:
            printf(
                "Unknown comparison type in expression '%s' in line %d\n",
                expr, line_number
            );
            exit(1);
            break;
    }

#ifdef DEBUG_PARSE
    printf("Expression to evaluate: '%s'\n", transformed_expression);
#endif
    if (!check_expression(transformed_expression)) {
        printf("Invalid expression '%s' on line %d\n", buffer, line_number);
        printf("%s\n", line);
        exit(1);
    }
    
    if (!evaluate_expression(transformed_expression, program)) {
        printf("Invalid expression '%s' in line %d\n", buffer, line_number);
        printf("%s\n", line);
        exit(1);
    }

    switch (comparison) {
        case LE:
        case GE:
            if (add_missing) remember_missing_reference(program, identifier.value);
            instruction = BRANCHNEG << OPERAND_BITS | address;
            program->memory[program->instruction_ptr++] = instruction;
            if (add_missing) remember_missing_reference(program, identifier.value);
            instruction = BRANCHZERO << OPERAND_BITS | address;
            program->memory[program->instruction_ptr++] = instruction;
            break;
        case LT:
        case GT:
            if (add_missing) remember_missing_reference(program, identifier.value);
            instruction = BRANCHNEG << OPERAND_BITS | address;
            program->memory[program->instruction_ptr++] = instruction;
            break;
        case EQ:
            if (add_missing) remember_missing_reference(program, identifier.value);
            instruction = BRANCHZERO << OPERAND_BITS | address;
            program->memory[program->instruction_ptr++] = instruction;
            break;
        case NE:
            size_t fakeaddr = program->instruction_ptr + 2;
            instruction = BRANCHZERO << OPERAND_BITS | fakeaddr;
            program->memory[program->instruction_ptr++] = instruction;
            if (add_missing) remember_missing_reference(program, identifier.value);
            instruction = BRANCH << OPERAND_BITS | address ;
            program->memory[program->instruction_ptr++] = instruction;
            break;
        default:
            printf(
                "Unknown comparison type in expression '%s' in line %d\n",
                expr, line_number
            );
            exit(1);
            break;
    }
}


bool evaluate_expression(char buffer[], struct Program *program) {
    /* Evaluates expression and places result into accumulator */
    word_t instruction;
    /* Tokenizing */
#ifdef DEBUG_EXPRESSION
    puts("Tokenizing expression and converting to postfix notation...");
#endif
    size_t num_tokens;
    struct ExpressionToken expression_tokens[MAX_TOKENS];
    if (!tokenize_expression(buffer, expression_tokens, &num_tokens)) {        
        printf("Invalid expression '%s'\n", buffer);
        return false;
    }

#ifdef DEBUG_EXPRESSION
    puts("Generating code...");
    puts("First pass. Allocating memory for variables and constants");
#endif
    for (size_t token_ptr = 0; token_ptr < num_tokens; token_ptr++) {
#ifdef DEBUG_EXPRESSION
        printf(
            "Token '%s' type '%c'\n", 
            expression_tokens[token_ptr].token, expression_tokens[token_ptr].token_type
        );
#endif
        if (expression_tokens[token_ptr].token_type == IDENTIFIER) {
            union Identifier identifier;
            if (check_integer(expression_tokens[token_ptr].token)) {
                identifier.value = atoi(expression_tokens[token_ptr].token);
                search_or_add_entry(program, identifier, CONST);
            } else if (check_identifier(expression_tokens[token_ptr].token)) {
                strcpy(identifier.name, expression_tokens[token_ptr].token);
                search_or_add_entry(program, identifier, VAR);
            } else {
                printf("%s is not a valid identifier\n", expression_tokens[token_ptr].token);
                return false;
            }
        }
    }

#ifdef DEBUG_EXPRESSION
    puts("Second pass. Generating code");
#endif

    word_t current_stack_pointer = program->stack_ptr;    
    size_t address;
    union Identifier identifier;
    for (size_t token_ptr = 0; token_ptr < num_tokens; token_ptr++) {
#ifdef DEBUG_EXPRESSION
        printf("Token %s\n", expression_tokens[token_ptr].token);
#endif
        if (expression_tokens[token_ptr].token_type == IDENTIFIER) {
            /* Load to stack */
            if (check_integer(expression_tokens[token_ptr].token)) {
                identifier.value = atoi(expression_tokens[token_ptr].token);
                address = search_entry(program, identifier, CONST);
            } else if (check_identifier(expression_tokens[token_ptr].token)) {
                strcpy(identifier.name, expression_tokens[token_ptr].token);
                address = search_entry(program, identifier, VAR);
            } else {
                printf("%s is not a valid identifier\n", expression_tokens[token_ptr].token);
                return false;
            }
            instruction = LOAD << OPERAND_BITS | address;
#ifdef DEBUG_EXPRESSION
            printf("From memory OPCODE %X", (word_t) instruction);
#endif
            program->memory[program->instruction_ptr++] = instruction;
            address = program->stack_ptr--;
            instruction = STORE << OPERAND_BITS | address;
#ifdef DEBUG_EXPRESSION
            printf(" to stack OPCODE %X\n", (word_t) instruction);
#endif
            program->memory[program->instruction_ptr++] = instruction;
        } else if (expression_tokens[token_ptr].token_type == OPERATION) {
            instruction = LOAD << OPERAND_BITS | ++(program->stack_ptr);
#ifdef DEBUG_EXPRESSION
            printf("From stack OPCODE %X to accumulator. ", (word_t) instruction);
#endif
            program->memory[program->instruction_ptr++] = instruction;
            switch (expression_tokens[token_ptr].token[0]) {
                case '+':
                    instruction = ADD;
                    break;
                case '-':
                    instruction = SUBTRACT;
                    break;
                case '*':
                    instruction = MULTIPLY;
                    break;
                case '/':
                    instruction = DIVIDE;
                    break;
                case '%':
                    instruction = REMAINDER;
                    break;
                case '^':
                    instruction = POWER;
                    break;
                default:
                    printf("Unknown operation '%c'\n", expression_tokens[token_ptr].token[0]);
                    return false;
            }
            instruction = instruction << OPERAND_BITS | ++(program->stack_ptr);
#ifdef DEBUG_EXPRESSION
            printf(" Arithmetic operation OPCODE %X.", (word_t) instruction);
#endif
            program->memory[program->instruction_ptr++] = instruction;
            instruction = STORE << OPERAND_BITS | program->stack_ptr--;
#ifdef DEBUG_EXPRESSION
            printf(" To stack %X\n", (word_t) instruction);
#endif
            program->memory[program->instruction_ptr++] = instruction;
        } else {
            printf("Wrong token '%s'\n", expression_tokens[token_ptr].token);
            return false;
        }
    }

    /* Placing result into accumulator */
    instruction = LOAD << OPERAND_BITS | ++(program->stack_ptr);
    program->memory[program->instruction_ptr++] = instruction;
#ifdef DEBUG_EXPRESSION
    printf("Placing result in accumulator OPCODE %X\n", instruction);
#endif
    if (program->stack_ptr != current_stack_pointer) {
        puts("Stack is in dirty state");
        return false;
    }
    return true;
}


bool check_expression (const char buffer[]) {
    bool notEmpty = false;
    for (const char *c = buffer; *c != '\0'; c++) {
        if (isalnum(*c)) notEmpty = true;
        if (*c == '=' || *c == '!' || *c == '<' || *c == '>') {
            printf("Illegal character '%c' in expression '%s'\n", *c, buffer);
            return false;
        }
    }
    if (!notEmpty) {
        puts("Empty expression.");
        return false;
    }
    return true;
}
