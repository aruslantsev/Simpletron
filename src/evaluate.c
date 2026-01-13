#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "evaluate.h"

#define DEBUG_EVAL


bool is_arithmetic_operation(const char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%';
}


bool is_parentheses(const char c) {
    return c == '(' || c == ')';
}


int compare_operations(const char op1, const char op2) {
    if ((op1 == '+' || op1 == '-') && (op2 == '*' || op2 == '/' || op2 == '%'))
        return -1;
    if ((op1 == '*' || op1 == '/' || op2 == '%') && (op2 == '+' || op2 == '-'))
        return 1;
    return 0;
}


bool tokenize_expression(char expression[], struct ExpressionToken tokens[], size_t *num_tokens) {
    /* Add trailing space. Last token will be saved automatically */
    strcat(expression, " ");

    struct ExpressionToken infix[MAX_TOKENS];
    size_t num_infix_tokens = 0;

#ifdef DEBUG_EVAL
    printf("Tokenizing expression '%s'\n", expression);
#endif

    char current_identifier[TOKEN_SIZE];
    size_t token_char_idx = 0;
    const char *c = expression;
    while (*c != '\0') {
        /* Operation, parentheses, blank, end of line are token separators */
        if (
            is_parentheses(*c)
            || is_arithmetic_operation(*c)
            || isblank(*c)
            || *c == '\n'
        ) {

            /* Unary sign */
            if (
                (
                    token_char_idx > 0
                    && (
                        (
                            num_infix_tokens == 1 
                            && infix[num_infix_tokens - 1].token_type == OPERATION
                        ) 
                        || (
                            num_infix_tokens > 2
                            && (
                                infix[num_infix_tokens - 2].token_type == OPERATION 
                                || (
                                    infix[num_infix_tokens - 2].token_type == PARENTHESES 
                                    && infix[num_infix_tokens - 2].token == '('
                                )
                            )
                            && infix[num_infix_tokens - 1].token_type == OPERATION
                            && (
                                infix[num_infix_tokens - 1].token[0] == '+'
                                || infix[num_infix_tokens - 1].token[0] == '-'
                            )
                        )
                    )
                )
                || (
                    *c == '(' 
                    && num_infix_tokens == 1 
                    && infix[num_infix_tokens - 1].token_type == OPERATION
                )
            ) {
                infix[num_infix_tokens - 1].token[1] = '1';
                infix[num_infix_tokens - 1].token[2] = '\0';
                infix[num_infix_tokens - 1].token_type = IDENTIFIER;
#ifdef DEBUG_EVAL
                printf(
                    "Unary operation detected. Converting previous token to '%s'\n", 
                    infix[num_infix_tokens - 1].token
                );
#endif
                num_infix_tokens++;
                infix[num_infix_tokens - 1] = (struct ExpressionToken) {
                    .token = "*", .token_type=OPERATION
                };
                
            }

            if (token_char_idx > 0) {
                /* Identifier */
                current_identifier[token_char_idx] = '\0';
                strcpy(infix[num_infix_tokens].token, current_identifier);
                infix[num_infix_tokens].token_type = IDENTIFIER;
#ifdef DEBUG_EVAL
                printf("Infix notation: adding token '%s'\n", infix[num_infix_tokens].token);
#endif
                num_infix_tokens++;
                token_char_idx = 0;
            } 
            if (is_parentheses(*c) || is_arithmetic_operation(*c)) {
                /* Save parentheses and operations: read single symbol to token */
                infix[num_infix_tokens].token[0] = *c;
                infix[num_infix_tokens].token[1] = '\0';
                if (is_arithmetic_operation(*c)) {
                    infix[num_infix_tokens].token_type = OPERATION;
                } else {
                    infix[num_infix_tokens].token_type = PARENTHESES;
                }
#ifdef DEBUG_EVAL
                printf("Infix notation: adding token '%s'\n", infix[num_infix_tokens].token);
#endif
                num_infix_tokens++;
            }
            c++;
        } else {
            while (
                !(
                    is_parentheses(*c)
                    || is_arithmetic_operation(*c)
                    || isblank(*c)
                    || *c == '\n'
                    || *c == '\0'
                )
            ) {
                /* Symbol. Save to token */
                current_identifier[token_char_idx++] = *c;
                c++;
            }
        }
    }

    /* Validation */
#ifdef DEBUG_EVAL
    puts("Tokenized");
    for (int ptr = 0; ptr < num_infix_tokens; ptr++) {
        printf("%s ", infix[ptr].token);
    }
    puts("");
    puts("Validating expression");
#endif

    int parentheses_count = 0;
    for (size_t ptr = 0; ptr < num_infix_tokens; ptr++) {
        if (infix[ptr].token_type == PARENTHESES) {
            if (infix[ptr].token[0] == '(') {
                parentheses_count++;
            } else if (infix[ptr].token[0] == ')') {
                parentheses_count--;
                if (parentheses_count < 0) {
                    puts("Error: Found closing parentheses without corresponding opening one.");
                    return false;
                }
            } else {
                printf("'%s' is not parentheses", infix[ptr].token);
                return false;
            }
                
        }
        if (ptr > 0) {
            if (
                infix[ptr - 1].token_type == IDENTIFIER 
                && infix[ptr].token_type == IDENTIFIER
            ) {
                printf(
                    "Got two identifiers without operation: '%s' '%s'\n", 
                    infix[ptr - 1].token, infix[ptr].token);
                return false;
            }
            if (infix[ptr - 1].token_type == OPERATION && infix[ptr].token_type == OPERATION) {
                printf(
                    "Got two operations without identifier: '%s' '%s'\n", 
                    infix[ptr - 1].token, infix[ptr].token
                );
                return false;
            }
            if (
                infix[ptr - 1].token_type == PARENTHESES 
                && infix[ptr].token_type == PARENTHESES 
                && infix[ptr - 1].token[0] != infix[ptr - 1].token[0]
            ) {
                printf(
                    "Got two different parentheses: '%s' '%s'\n", 
                    infix[ptr - 1].token, infix[ptr].token
                );
                return false;
            }
        }
    }

#ifdef DEBUG_EVAL
    puts("Converting to postfix notation");
#endif

    size_t postfix_tokens = 0;
    struct ExpressionToken stack[MAX_TOKENS];
    size_t stack_ptr = 0;
    for(size_t infix_ptr = 0; infix_ptr < num_infix_tokens; infix_ptr++) {
#ifdef DEBUG_EVAL
        printf("Token '%s', type '%c'\n",infix[infix_ptr].token, infix[infix_ptr].token_type);
#endif
        if (infix[infix_ptr].token_type == IDENTIFIER) {
#ifdef DEBUG_EVAL
            puts("To postfix");
#endif
            tokens[postfix_tokens++] = infix[infix_ptr];
        } else if (infix[infix_ptr].token_type == OPERATION) {
            while (
                stack_ptr > 0 
                && stack[stack_ptr - 1].token_type == OPERATION 
                && compare_operations(
                    infix[infix_ptr].token[0], stack[stack_ptr - 1].token[0]
                ) < 0
            ) {
#ifdef DEBUG_EVAL
                printf(
                    "Moving from stack to output queue token '%s' type '%c'\n", 
                    stack[stack_ptr - 1].token, stack[stack_ptr - 1].token_type
                );
#endif
                tokens[postfix_tokens++] = stack[--stack_ptr];
            }
#ifdef DEBUG_EVAL
            printf(
                "Placing token '%s', type '%c' to stack\n",
                infix[infix_ptr].token, infix[infix_ptr].token_type
            );
#endif
            stack[stack_ptr++] = infix[infix_ptr];
        } else if (infix[infix_ptr].token_type == PARENTHESES) {
            if (infix[infix_ptr].token[0] == '(') {
#ifdef DEBUG_EVAL
                puts("To stack (opening parentheses).");
#endif
                stack[stack_ptr++] = infix[infix_ptr];
            } else {
                while (stack_ptr > 0 && stack[stack_ptr - 1].token_type != PARENTHESES) {
#ifdef DEBUG_EVAL
                    printf(
                        "Moving from stack to output queue token '%s' type '%c'\n", 
                        stack[stack_ptr - 1].token, stack[stack_ptr - 1].token_type
                    );
#endif
                    tokens[postfix_tokens++] = stack[--stack_ptr];
                }
#ifdef DEBUG_EVAL
                puts("Pop opening parentheses");
#endif
                stack_ptr--;
            }
        }
    }

#ifdef DEBUG_EVAL
    if (stack_ptr > 0) {
        puts("Stack is not empty");
    }
#endif
    while (stack_ptr > 0 && stack[stack_ptr - 1].token_type != PARENTHESES) {
#ifdef DEBUG_EVAL
        printf(
            "Moving from stack to output queue token '%s' type '%c'\n", 
            stack[stack_ptr - 1].token, stack[stack_ptr - 1].token_type
        );
#endif
        tokens[postfix_tokens++] = stack[--stack_ptr];
    }
#ifdef DEBUG_EVAL
    puts("Resulting postfix notation");
    for (int ptr = 0; ptr < postfix_tokens; ptr++) {
        printf("%s ", tokens[ptr].token);
    }
    puts("");
#endif
    *num_tokens = postfix_tokens;
    return true;
}
