#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simpletron.h"
#include "translator.h"
#include "evaluate.h"


#define DEBUG_LOOKUP_LIST
#define DEBUG_MISSING_REF_LIST
#define DEBUG_PARSE
#define DEBUG_EXPRESSION
// #define DEBUG


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
    struct LookupListEntry *lookup_list_pointer = program->lookup_list;
    while (lookup_list_pointer != NULL) {
#ifdef DEBUG_LOOKUP_LIST
        printf("Scanning object: %c", lookup_list_pointer->type);
        if (lookup_list_pointer->type == VAR) {
            printf(" %s", lookup_list_pointer->identifier.name);
        } else {
            printf(" %d", lookup_list_pointer->identifier.value);
        }
        printf(
            ", address %ld (%0X)\n",
            lookup_list_pointer->address, (uword_t) lookup_list_pointer->address
        );
#endif
        if (
            (
                type == VAR
                && type == lookup_list_pointer->type
                && strcmp(lookup_list_pointer->identifier.name, identifier.name) == 0
            )
            || (
                (type == CONST || type == LINE)
                && type == lookup_list_pointer->type
                && lookup_list_pointer->identifier.value == identifier.value
            )
        ) {
#ifdef DEBUG_LOOKUP_LIST
            printf(
                "Found, address: %ld (%0X)\n",
                lookup_list_pointer->address, (uword_t) lookup_list_pointer->address
            );
#endif
            return lookup_list_pointer->address;
        }
        lookup_list_pointer = lookup_list_pointer->next;
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
    struct LookupListEntry *new_entry = (struct LookupListEntry *) malloc(
        sizeof(struct LookupListEntry)
    );
    if (new_entry == NULL) {
        puts("Error. Can't allocate memory");
        exit(1);
    }
    new_entry->identifier = identifier;
    new_entry->type = type;
    if (new_entry->type == LINE) {
        new_entry->address = program->instruction_ptr;
    } else {
        new_entry->address = program->constants_ptr;
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
    new_entry->next = NULL;
    if (program->lookup_list == NULL) {
        program->lookup_list = new_entry;
    } else {
        struct LookupListEntry *lookup_list_ptr = program->lookup_list;
        while (lookup_list_ptr->next != NULL) {
            lookup_list_ptr = lookup_list_ptr->next;
        }
        lookup_list_ptr->next = new_entry;
    }
#ifdef DEBUG_LOOKUP_LIST
    printf("Added object: %c", new_entry->type);
    if (new_entry->type == VAR) {
        printf(" %s", new_entry->identifier.name);
    } else {
        printf(" %d", new_entry->identifier.value);
    }
    printf(
        ", address %ld (%0X)\n", new_entry->address, (uword_t) new_entry->address
    );
#endif
    return new_entry->address;
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
    struct MissingRefListEntry *entry = (struct MissingRefListEntry *) malloc(
        sizeof(struct MissingRefListEntry)
    );
    struct MissingRefListEntry *missing_ref_list_ptr;
    *entry = (struct MissingRefListEntry) {
        .label=identifier, .address=program->instruction_ptr, .next=NULL
    };
    if (program->missing_ref_list == NULL) {
        program->missing_ref_list = entry;
    } else {
        missing_ref_list_ptr = program->missing_ref_list;
        while (missing_ref_list_ptr->next != NULL) {
#ifdef DEBUG_MISSING_REF_LIST
            printf(
                "Found record for missing reference to row %d at address %ld (%0X)\n",
                missing_ref_list_ptr->label,
                missing_ref_list_ptr->address,
                (uword_t) missing_ref_list_ptr->address
            );
#endif
            missing_ref_list_ptr = missing_ref_list_ptr->next;
        }
        missing_ref_list_ptr->next = entry;
#ifdef DEBUG_MISSING_REF_LIST
        puts("Done.");
#endif
    }
}


void init_program(struct Program *program) {
    program->lookup_list = NULL;
    program->missing_ref_list = NULL;
    for (size_t ptr = 0; ptr < MEMORY_SIZE; ptr++) {
        program->memory[ptr] = 0;
    }
    program->instruction_ptr = 0;
    program->constants_ptr = MEMORY_SIZE - 1;
    program->stack_ptr = program->constants_ptr - NUM_CONSTANTS;
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
    // Null-terminate the modified string
    s1[j] = '\0';
    // Trim a single trailing space if it exists
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
    if (token == NULL) return;  // Empty line
    if (strcmp("rem", token) == 0) return;  // Skip comments
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

    strcpy(buffer, line);
    strtok(buffer, " ");  // Skip line number
    strtok(NULL, " ");  // Skip keyword
    char *token = strtok(NULL, " ,");  // Tokenize remaining string by space of comma
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
        program->memory[program->instruction_ptr++] = instruction;
        token = strtok(NULL, " ,");
    }
}


void parse_print(struct Program *program, char line[], const int line_number) {
    word_t instruction;
    char buffer[BUFFER_SIZE];
    union Identifier identifier;
    size_t address;

    strcpy(buffer, line);
    strtok(buffer, " ");  // Skip line number
    strtok(NULL, " ");  // Skip keyword
    char *token = strtok(NULL, " ,");  // Tokenize remaining string by space of comma
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

    strcpy(buffer, line);
    strtok(buffer, " ");  // Skip line number
    strtok(NULL, " ");  // Skip keyword
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
    program->memory[program->instruction_ptr++] = instruction;
}


void parse_let(struct Program *program, char line[], const int line_number) {
    char buffer[BUFFER_SIZE];
    union Identifier identifier;

    strcpy(buffer, line);
    char *buffer_start = buffer;
    strtok(buffer, " ");  // Skip line number
    strtok(NULL, " ");  // Skip keyword

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
    size_t str_ptr = token - buffer_start + strlen(token);  // End of variable name
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

    strcpy(buffer, line);
    const char *buffer_start = buffer;
    strtok(buffer, " ");  // Skip line number
    char *token = strtok(NULL, " ");  // Skip keyword

    // Copy from start of left expression
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
    condition_end += 4;  // Skip GOTO keyword
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

    printf("%s\n", transformed_expression);
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
    struct ExpressionToken *expr_start = tokenize_expression(buffer);
    if (expr_start == NULL) {
        printf("Invalid expression '%s'\n", buffer);
        return false;
    }

    struct ExpressionToken *current_token = expr_start;
#ifdef DEBUG_EXPRESSION
    puts("Generating code...");
    puts("First pass. Allocating memory for variables and constants");
#endif
    while (current_token != NULL) {
#ifdef DEBUG_EXPRESSION
        printf("Token '%s' type '%c'\n", current_token->token, current_token->token_type);
#endif
        if (current_token->token_type == IDENTIFIER) {
            union Identifier identifier;
            if (check_integer(current_token->token)) {
                identifier.value = atoi(current_token->token);
                search_or_add_entry(program, identifier, CONST);
            } else if (check_identifier(current_token->token)) {
                strcpy(identifier.name, current_token->token);
                search_or_add_entry(program, identifier, VAR);
            } else {
                printf("%s is not a valid identifier\n", current_token->token);
                return false;
            }
        }
        current_token = current_token->next;
    }

#ifdef DEBUG_EXPRESSION
    puts("Second pass. Generating code");
#endif
    /* Generate code */
    word_t current_stack_pointer = program->stack_ptr;
    current_token = expr_start;
    
    size_t address;
    while (current_token != NULL) {
#ifdef DEBUG_EXPRESSION
        printf("Token %s\n", current_token->token);
#endif
        if (current_token->token_type == IDENTIFIER) {
            // Load to stack
            union Identifier identifier;
            if (check_integer(current_token->token)) {
                identifier.value = atoi(current_token->token);
                address = search_entry(program, identifier, CONST);
            } else if (check_identifier(current_token->token)) {
                strcpy(identifier.name, current_token->token);
                address = search_entry(program, identifier, VAR);
            } else {
                printf("%s is not a valid identifier\n", current_token->token);
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
        } else if (current_token->token_type == OPERATION) {
            instruction = LOAD << OPERAND_BITS | ++(program->stack_ptr);
#ifdef DEBUG_EXPRESSION
            printf("From stack OPCODE %X to accumulator. ", (word_t) instruction);
#endif
            program->memory[program->instruction_ptr++] = instruction;
            switch (current_token->token[0]) {
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
                default:
                    printf("Unknown operation '%c'\n", current_token->token[0]);
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
            printf("Wrong token '%s'\n", current_token->token);
            return false;
        }
        current_token = current_token->next;
    }
    /* Cleanup */
    current_token = expr_start;
    while (current_token != NULL) {
        expr_start = current_token->next;
        free(current_token);
        current_token = expr_start;
    }
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
