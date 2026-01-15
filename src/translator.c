#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simpletron.h"
#include "translator.h"
#include "evaluate.h"



void init_program(struct Program *program) {
    for (size_t ptr = 0; ptr < MEMORY_SIZE; ptr++) {
        program->memory[ptr] = 0;
    }
    program->instruction_ptr = 0;
    program->constants_ptr = MEMORY_SIZE - 1;
    program->stack_ptr = 0;
    program->lookup_list_size = 0;
    program->missing_ref_list_size = 0;
    program->for_ptr = 0;
    program->stack_offset_list_size = 0;
}


/* Searches object's memory address in lookup list */
word_t search_entry(
    struct Program *program,
    const union Identifier identifier,
    const enum EntryType type)
{
    for (
        size_t lookup_list_ptr = 0; lookup_list_ptr < program->lookup_list_size; lookup_list_ptr++
    ) {
        /* Check for matching type and name (for variable) or value (for constant or line number )*/
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
            return program->lookup_list[lookup_list_ptr].address;
        }
    }
    return OBJ_NOT_FOUND;
}

/* Adds object to lookup list */
word_t add_entry(
    struct Program *program,
    const union Identifier identifier,
    const enum EntryType type) {
    program->lookup_list[program->lookup_list_size].identifier = identifier;
    program->lookup_list[program->lookup_list_size].type = type;

    if (type == LINE) {
        program->lookup_list[program->lookup_list_size].address = program->instruction_ptr;
    } else {
        /* Reserving memory */
        program->lookup_list[program->lookup_list_size].address = program->constants_ptr;
        if (type == CONST) {
            /* Writing constant value to memory */
            program->memory[program->constants_ptr] = identifier.value;
        }
        program->constants_ptr--;
    }
    program->lookup_list_size++;
    return program->lookup_list[program->lookup_list_size - 1].address;
}

/* Searches object's memory address in lookup list 
 * and adds object to lookup list if was not found */
word_t search_or_add_entry(
    struct Program *program,
    const union Identifier identifier,
    const enum EntryType type) {
    const word_t address = search_entry(program, identifier, type);
    if (address != OBJ_NOT_FOUND) {
        return address;
    }
    return add_entry(program, identifier, type);
}

/* Saves reference to line, that was not still processed */
void remember_missing_reference(struct Program *program, const int identifier) {
    program->missing_ref_list[program->missing_ref_list_size++] = (struct MissingRefListEntry) {
        .label=identifier, .address=program->instruction_ptr
    };
}


/* Saves stack offset for instruction at specified address */
void remember_stack_offset(struct Program *program, const word_t address, const word_t offset) {
    program->stack_offset_list[program->stack_offset_list_size++] = (struct StackOffsetEntry) {
        .address=address, .offset=offset
    };
}


/* Checks if provided string can be variable name */
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


/* Checks if provided string is integer value */
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


/* Removes leading and trailing spaces and replace duplicated spaces in s2,
 * saves result in s1. s2 remains unmodified */
void strip(char s1[], const char s2[]) {
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


bool parse_line(struct Program *program, char line[], const int line_number) {
    char buffer[BUFFER_SIZE];
    union Identifier identifier;
    strcpy(buffer, line);

    /* First token: line number */
    char *token = strtok(buffer, " ");
    if (!check_integer(token) || (identifier.value = atoi(token)) < 0) {
        printf("Bad or negative line number on line %d\n", line_number);
        return false;
    }
    if (search_entry(program, identifier, LINE) != OBJ_NOT_FOUND) {
        printf(
            "Error: duplicated line number '%d' on line %d\n",
            identifier.value, line_number
        );
        return false;
    }
    if (add_entry(program, identifier, LINE) == OBJ_NOT_FOUND) {
        printf("Unknown error on line %d\n", line_number);
        return false;
    }

    /* Second token: keyword */
    token = strtok(NULL, " ");
    if (token == NULL) return true;  /* Empty line */
    if (strcmp("rem", token) == 0) return true;  /* Skip comments */
    if (strcmp("input", token) == 0) return parse_input(program, line, line_number);
    else if (strcmp("print", token) == 0) return parse_print(program, line, line_number);
    else if (strcmp("let", token) == 0) return parse_let(program, line, line_number);
    else if (strcmp("goto", token) == 0) return parse_goto(program, line, line_number);
    else if (strcmp("if", token) == 0) return parse_if(program, line, line_number);
    else if (strcmp("for", token) == 0) return parse_for(program, line, line_number);
    else if (strcmp("next", token) == 0) return parse_for_end(program, line, line_number);
    else if (strcmp("end", token) == 0) {
        const word_t instruction = HALT << OPERAND_BITS;
        program->memory[program->instruction_ptr++] = instruction;
        return true;
    } else {
        printf("Wrong token '%s' on line %d\n", token, line_number);
        return false;
    }
    return false;
}


bool parse_input(struct Program *program, char line[], const int line_number) {
    word_t instruction, address;
    char buffer[BUFFER_SIZE];
    union Identifier identifier;

    strcpy(buffer, line);
    strtok(buffer, " ");  /* Skip line number */
    strtok(NULL, " ");  /* Skip keyword */
    char *token = strtok(NULL, " ,");  /* Tokenize remaining string by space of comma */
    if (token == NULL) {
        printf("Missing variable name after INPUT keyword on line %d\n", line_number);
        return false;
    }
    while (token != NULL) {
        if (!check_identifier(token)) {
            printf("Wrong variable name %s on line %d\n", token, line_number);
            return false;
        }
        strncpy(identifier.name, token, IDENTIFIER_SIZE - 1);
        address = search_or_add_entry(program, identifier, VAR);
        if (address == OBJ_NOT_FOUND) {
            printf("Unknown error on line %d\n", line_number);
            return false;
        }
        instruction = READ << OPERAND_BITS | address;
        program->memory[program->instruction_ptr++] = instruction;
        token = strtok(NULL, " ,");
    }
    return true;
}


bool parse_print(struct Program *program, char line[], const int line_number) {
    word_t instruction, address;
    char buffer[BUFFER_SIZE];
    union Identifier identifier;

    strcpy(buffer, line);
    strtok(buffer, " ");  /* Skip line number */
    strtok(NULL, " ");  /* Skip keyword */
    char *token = strtok(NULL, " ,");  /* Tokenize remaining string by space of comma */
    if (token == NULL) {
        printf("Missing variable name after PRINT keyword on line %d\n", line_number);
        return false;
    }
    while (token != NULL) {
        if (!check_identifier(token)) {
            printf("Wrong variable name %s on line %d\n", token, line_number);
            return false;
        }
        strncpy(identifier.name, token, IDENTIFIER_SIZE - 1);
        address = search_or_add_entry(program, identifier, VAR);
        if (address == OBJ_NOT_FOUND) {
            printf("Unknown error on line %d\n", line_number);
            return false;
        }
        instruction = WRITE << OPERAND_BITS | address;
        program->memory[program->instruction_ptr++] = instruction;
        token = strtok(NULL, " ,");
    }
    return true;
}


bool parse_goto(struct Program *program, char line[], const int line_number) {
    word_t instruction, address;
    char buffer[BUFFER_SIZE];
    union Identifier identifier;

    strcpy(buffer, line);
    strtok(buffer, " ");  /* Skip line number */
    strtok(NULL, " ");  /* Skip keyword */
    char *token = strtok(NULL, " ");
    if (token == NULL) {
        printf("Missing line number after GOTO keyword on line %d\n", line_number);
        return false;
    }
    if (!check_integer(token) || (identifier.value = atoi(token)) < 0) {
        printf("Bad or negative line number on line %d\n", line_number);
        return false;
    }
    token = strtok(NULL, " ");
    if (token != NULL) {
        printf("Error: multiple line numbers after GOTO keyword on line %d\n", line_number);
        return false;
    }
    if ((address = search_entry(program, identifier, LINE)) == OBJ_NOT_FOUND) {
        remember_missing_reference(program, identifier.value);
        address = 0;
    }
    instruction = BRANCH << OPERAND_BITS | address;
    program->memory[program->instruction_ptr++] = instruction;
    return true;
}


bool parse_let(struct Program *program, char line[], const int line_number) {
    char buffer[BUFFER_SIZE];
    union Identifier identifier;

    strcpy(buffer, line);
    char *buffer_start = buffer;
    strtok(buffer, " ");  /* Skip line number */
    strtok(NULL, " ");  /* Skip keyword */

    char *token = strtok(NULL, " =");
    if (token == NULL || !check_identifier(token)) {
        printf("Missing or bad variable name on line %d\n", line_number);
        return false;
    }
    strncpy(identifier.name, token, IDENTIFIER_SIZE - 1);
    const word_t address = search_or_add_entry(program, identifier, VAR);
    if (address == OBJ_NOT_FOUND) {
        printf("Unknown error on line %d\n", line_number);
        return false;
    }
    size_t str_ptr = token - buffer_start + strlen(token);  /* End of variable name */
    /* Check space between variable name and '='. Should be only spaces */
    for (; line[str_ptr] != '='; str_ptr++) {
        /* End of line or non-blank symbol before '='' */
        if (line[str_ptr] == '\0' || !isblank(line[str_ptr])) {
            printf("Unexpected character on line %d\n", line_number);
            return false;
        }
    }
    /* At '=' now. Switch to the next char */
    str_ptr++; 
    /* Copy expression to evaluate */
    strcpy(buffer, &line[str_ptr]);
    strip(buffer, buffer);

    if (!check_expression(buffer) || !evaluate_expression(buffer, program)) {
        printf("Invalid expression '%s' on line %d\n", buffer, line_number);
        return false;
    }
    const word_t instruction = STORE << OPERAND_BITS | address;
    program->memory[program->instruction_ptr++] = instruction;
    return true;
}


bool parse_if(struct Program *program, char line[], const int line_number) {
    word_t instruction, address;
    char buffer[BUFFER_SIZE];
    union Identifier identifier;

    strcpy(buffer, line);
    const char *buffer_start = buffer;
    strtok(buffer, " ");  /* Skip line number */
    char *token = strtok(NULL, " ");  /* Skip keyword */

    /* Copy from start of left expression */
    strcpy(buffer, &line[token - buffer_start + strlen(token)]);
    strip(buffer, buffer);

    char *condition_end = strstr(buffer, "goto");
    if (condition_end == NULL) {
        printf("Missing GOTO in IF statement on line %d\n", line_number);
        return false;
    }
    char expr[BUFFER_SIZE];
    strncpy(expr, buffer, condition_end - buffer);
    expr[condition_end - buffer] = '\0';
    condition_end += 4;  /* Skip GOTO keyword */
    token = strtok(condition_end, " ");
    if (token == NULL || !check_integer(token) || (identifier.value = atoi(token)) < 0) {
        printf("Missing or bad line number after IF..GOTO keyword on line %d\n", line_number);
        return false;
    }

    bool add_missing = false;
    if ((address = search_entry(program, identifier, LINE)) == OBJ_NOT_FOUND) {
        add_missing = true;
        address = 0;
    }

    token = strtok(NULL, " ");
    if (token != NULL) {
        printf(
            "Multiple line numbers after IF..GOTO keyword on line %d\n",
            line_number
        );
        return false;
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
        printf("Can't find comparison in expression '%s' on line %d\n", expr, line_number);
        return false;
    }

    strncpy(left_expression, expr, comparison_position - expr);
    left_expression[comparison_position - expr] = '\0';
    strip(left_expression, left_expression);
    strcpy(
        right_expression,
        comparison_position + ((comparison == LT || comparison == GT)? 1 : 2)
    );
    strip(right_expression, right_expression);

    switch (comparison) {
        case LE:
        case LT:
        case EQ:
        case NE:
            sprintf(
                transformed_expression, "(%s)-(%s)", left_expression, right_expression
            );
            break;
        case GE:
        case GT:
            sprintf(
                transformed_expression, "(%s)-(%s)", right_expression, left_expression
            );
            break;
        default:
            printf(
                "Unknown comparison type in expression '%s' on line %d\n",
                expr, line_number
            );
            return false;
    }

    if (
        !check_expression(transformed_expression) 
        || !evaluate_expression(transformed_expression, program)
    ) {
        printf("Invalid expression '%s' on line %d\n", buffer, line_number);
        return false;
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
            /* If equal, skip next instruction */
            size_t fakeaddr = program->instruction_ptr + 2;
            instruction = BRANCHZERO << OPERAND_BITS | fakeaddr;
            program->memory[program->instruction_ptr++] = instruction;
            /* Otherwise (not equal) go to specified line */
            if (add_missing) remember_missing_reference(program, identifier.value);
            instruction = BRANCH << OPERAND_BITS | address ;
            program->memory[program->instruction_ptr++] = instruction;
            break;
        default:
            printf(
                "Unknown comparison type in expression '%s' on line %d\n",
                expr, line_number
            );
            return false;
    }
    return true;
}


bool parse_for(struct Program *program, char line[], const int line_number) {
    word_t instruction;
    char buffer[BUFFER_SIZE];
    union Identifier identifier;
    word_t var_address, from_address, to_address, step_address;

    strcpy(buffer, line);
    const char *buffer_start = buffer;
    strtok(buffer, " ");  /* Skip line number */
    strtok(NULL, " ");  /* Skip keyword */
    char *token = strtok(NULL, " ");  /* Identifier */
    if (token == NULL || !check_identifier(token)) {
        printf("Missing or bad identifier '%s' on line %d\n", token, line_number);
        printf("%s\n", line);
        exit(EXIT_FAILURE);
    }
    strncpy(identifier.name, token, IDENTIFIER_SIZE - 1);
    var_address = search_or_add_entry(program, identifier, VAR);

    size_t str_ptr = token - buffer_start + strlen(token);  /* End of variable name */
    /* Check space between variable name and '='. Should be only spaces */
    for (; line[str_ptr] != '='; str_ptr++) {
        /* End of line before assignment */
        if (line[str_ptr] == '\0' || !isblank(line[str_ptr])) {
            printf("Unexpected character on line %d\n", line_number);
            return false;
        }
    }
    /* At '=' now */
    str_ptr++;
    strcpy(buffer, &line[str_ptr]);
    strip(buffer, buffer);

    token = strtok(buffer, " ");
    /* Start value */
    if (token == NULL) {
        printf("Missing start value on line %d\n", line_number);
        return false;
    }
    from_address = OBJ_NOT_FOUND;
    if (check_integer(token)) {
        identifier.value = atoi(token);
        from_address = search_or_add_entry(program, identifier, CONST);
    } else if (check_identifier(token)) {
        strcpy(identifier.name, token);
        from_address = search_entry(program, identifier, VAR);
    } 
    if (from_address == OBJ_NOT_FOUND) {
        printf("Bad identifier '%s' on line %d\n", token, line_number);
        return false;
    }
    /* to */
    token = strtok(NULL, " ");
    if (token == NULL || strcmp("to", token) != 0) {
        printf("Missing TO after FOR on line %d\n", line_number);
        return false;
    }
    /* End value */
    token = strtok(NULL, " ");
    if (token == NULL) {
        printf("Missing end value on line %d\n", line_number);
        printf("%s\n", line);
        exit(EXIT_FAILURE);
    }
    to_address = OBJ_NOT_FOUND;
    if (check_integer(token)) {
        identifier.value = atoi(token);
        to_address = search_or_add_entry(program, identifier, CONST);
    } else if (check_identifier(token)) {
        strcpy(identifier.name, token);
        to_address = search_entry(program, identifier, VAR);
    } 
    if (to_address == OBJ_NOT_FOUND) {
        printf("Bad identifier '%s' on line %d\n", token, line_number);
        return false;
    }
    token = strtok(NULL, " ");
    if (token == NULL) {
        identifier.value = 1;
        step_address = search_or_add_entry(program, identifier, CONST);
    } else {
        if (strcmp("step", token) != 0) {
            printf("Unexpected token '%s' in FOR on line %d\n", token, line_number);
            return false;
        }
        token = strtok(NULL, " ");
        if (token == NULL) {
            printf("Missing step value on line %d\n", line_number);
            return false;
        }
        step_address = OBJ_NOT_FOUND;
        if (check_integer(token)) {
            identifier.value = atoi(token);
            step_address = search_or_add_entry(program, identifier, CONST);
        } else if (check_identifier(token)) {
            strcpy(identifier.name, token);
            step_address = search_entry(program, identifier, VAR);
        } 
        if (step_address == OBJ_NOT_FOUND) {
            printf("Bad identifier '%s' on line %d\n", token, line_number);
            return false;
        }
    }
    instruction = LOAD << OPERAND_BITS | from_address;
    program->memory[program->instruction_ptr++] = instruction;
    instruction = STORE << OPERAND_BITS | var_address;
    program->memory[program->instruction_ptr++] = instruction;

    /* Save to stack current instruction pointer. Will return here on NEXT keyword */
    program->for_stack[program->for_ptr++] = (struct ForEntry) {
        .cycle_begin_address=program->instruction_ptr,
        .var_address=var_address,
        .to_address=to_address, 
        .step_address=step_address
    };
    return true;
}


bool parse_for_end(struct Program *program, char line[], const int line_number) {
    word_t instruction;
    char buffer[BUFFER_SIZE];

    strcpy(buffer, line);
    strtok(buffer, " ");  /* Skip line number */
    strtok(NULL, " ");  /* Skip keyword */
    char *token = strtok(NULL, " ");  /* Tokenize remaining string by space of comma */
    if (token != NULL) {
        printf("Extra symbols after NEXT keyword on line %d\n", line_number);
        return false;
    }

    const struct ForEntry entry = program->for_stack[--program->for_ptr];
    /* increment and save variable */
    instruction = LOAD << OPERAND_BITS | entry.var_address;
    instruction = program->memory[program->instruction_ptr++] = instruction;
    instruction = ADD << OPERAND_BITS | entry.step_address;
    instruction = program->memory[program->instruction_ptr++] = instruction;
    instruction = STORE << OPERAND_BITS | entry.var_address;
    instruction = program->memory[program->instruction_ptr++] = instruction;
    /* Compare variable and end value */
    instruction = LOAD << OPERAND_BITS | entry.to_address;
    instruction = program->memory[program->instruction_ptr++] = instruction;
    instruction = SUBTRACT << OPERAND_BITS | entry.var_address;
    instruction = program->memory[program->instruction_ptr++] = instruction;
    instruction = BRANCHZERO << OPERAND_BITS | entry.cycle_begin_address;
    instruction = program->memory[program->instruction_ptr++] = instruction;
    instruction = BRANCHNEG << OPERAND_BITS | entry.cycle_begin_address;
    instruction = program->memory[program->instruction_ptr++] = instruction;
    return true;
}


bool evaluate_expression(char buffer[], struct Program *program) {
    /* Evaluates expression and places result into accumulator */
    word_t instruction;
    /* Tokenizing */
    size_t num_tokens;
    struct ExpressionToken expression_tokens[MAX_TOKENS];
    if (!tokenize_expression(buffer, expression_tokens, &num_tokens)) {        
        printf("Invalid expression '%s'\n", buffer);
        return false;
    }

    /* Generating code */
    size_t current_stack_pointer = program->stack_ptr;    
    word_t address;
    union Identifier identifier;
    for (size_t token_ptr = 0; token_ptr < num_tokens; token_ptr++) {
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
            if (address == OBJ_NOT_FOUND) {
                printf("Identifier '%s' was not found\n", expression_tokens[token_ptr].token);
            }
            /* From memory */
            instruction = LOAD << OPERAND_BITS | address;
            program->memory[program->instruction_ptr++] = instruction;
            /* To stack */
            address = program->stack_ptr++;
            remember_stack_offset(program, program->instruction_ptr, address);
            instruction = STORE << OPERAND_BITS;
            program->memory[program->instruction_ptr++] = instruction;
        } else if (expression_tokens[token_ptr].token_type == OPERATION) {
            /* From stack to accumulator */
            address = --(program->stack_ptr);
            remember_stack_offset(program, program->instruction_ptr, address);
            instruction = LOAD << OPERAND_BITS;
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
            /* Second operand */
            address = --(program->stack_ptr);
            remember_stack_offset(program, program->instruction_ptr, address);
            instruction = instruction << OPERAND_BITS;

            /* Place result from accumulator to stack */
            program->memory[program->instruction_ptr++] = instruction;
            address = program->stack_ptr++;
            remember_stack_offset(program, program->instruction_ptr, address);
            instruction = STORE << OPERAND_BITS;
            program->memory[program->instruction_ptr++] = instruction;
        } else {
            printf("Wrong token '%s'\n", expression_tokens[token_ptr].token);
            return false;
        }
    }

    /* Placing result into accumulator */
    address = --(program->stack_ptr);
    remember_stack_offset(program, program->instruction_ptr, address);
    instruction = LOAD << OPERAND_BITS;
    program->memory[program->instruction_ptr++] = instruction;
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
