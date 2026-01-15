#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "evaluate.h"


bool is_arithmetic_operation(const char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^';
}


bool is_parenthesis(const char c) {
    return c == '(' || c == ')';
}


int compare_operations(const char op1, const char op2) {
    if (op1 == '^' && op2 != '^')
        return 1;
    if (op1 != '^' && op2 == '^')
        return -1;
    if ((op1 == '+' || op1 == '-') && (op2 == '*' || op2 == '/' || op2 == '%'))
        return -1;
    if ((op1 == '*' || op1 == '/' || op1 == '%') && (op2 == '+' || op2 == '-'))
        return 1;
    return 0;
}


bool tokenize_expression(char expression[], struct ExpressionToken tokens[], size_t *num_tokens) {
    /* Add trailing space. Last token will be saved automatically */
    strcat(expression, " ");

    struct ExpressionToken infix[MAX_TOKENS];
    size_t num_infix_tokens = 0;
    char current_identifier[TOKEN_SIZE];
    size_t token_char_idx = 0;

    const char *c = expression;
    while (*c != '\0') {
        /* Operation, parenthesis, blank, end of line are token separators */
        if (
            is_parenthesis(*c)
            || is_arithmetic_operation(*c)
            || isblank(*c)
            || *c == '\n'
        ) {
            /* Unary sign */
            if (
                (
                    token_char_idx > 0
                    && (
                        /* First token is unary sign, bad sequences will be detected later*/
                        (
                            num_infix_tokens == 1 
                            && infix[num_infix_tokens - 1].token_type == OPERATION
                        ) 
                        || (
                            /* Operation, unary sign, identifier or 
                             * opening parenthesis, unary sign, identifier */
                            num_infix_tokens > 2
                            && (
                                infix[num_infix_tokens - 2].token_type == OPERATION 
                                || (
                                    infix[num_infix_tokens - 2].token_type == PARENTHESIS 
                                    && infix[num_infix_tokens - 2].token[0] == '('
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
                /* Opening parenthesis can be the second token, while the first one is unary sign */
                || (
                    *c == '(' 
                    && num_infix_tokens == 1 
                    && infix[num_infix_tokens - 1].token_type == OPERATION
                )
            ) {
                /* Change operation to identifier (+1 or -1) and add after it extra multiplication 
                 * So, e.g. -7 converts to (-1) * 7, -x converts to (-1 * x) */
                infix[num_infix_tokens - 1].token[1] = '1';
                infix[num_infix_tokens - 1].token[2] = '\0';
                infix[num_infix_tokens - 1].token_type = IDENTIFIER;
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
                num_infix_tokens++;
                token_char_idx = 0;
            } 
            if (is_parenthesis(*c) || is_arithmetic_operation(*c)) {
                /* Save parentheses and operations: read single symbol to token */
                infix[num_infix_tokens].token[0] = *c;
                infix[num_infix_tokens].token[1] = '\0';
                if (is_arithmetic_operation(*c)) {
                    infix[num_infix_tokens].token_type = OPERATION;
                } else {
                    infix[num_infix_tokens].token_type = PARENTHESIS;
                }
                num_infix_tokens++;
            }
            c++;
        } else {
            while (
                !(
                    is_parenthesis(*c)
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
    if (infix[0].token_type == OPERATION) {
        puts("Error: expression starts with operation");
        return false;
    }
    int parentheses_count = 0;
    for (size_t ptr = 0; ptr < num_infix_tokens; ptr++) {
        if (infix[ptr].token_type == PARENTHESIS) {
            if (infix[ptr].token[0] == '(') {
                parentheses_count++;
            } else if (infix[ptr].token[0] == ')') {
                parentheses_count--;
                if (parentheses_count < 0) {
                    puts("Error: Found closing parenthesis without corresponding opening one.");
                    return false;
                }
            } else {
                printf("'%s' is not parenthesis\n", infix[ptr].token);
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
                infix[ptr - 1].token_type == PARENTHESIS 
                && infix[ptr].token_type == PARENTHESIS 
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

    /* To postfix */
    size_t postfix_tokens = 0;
    struct ExpressionToken stack[MAX_TOKENS];
    size_t stack_ptr = 0;
    for(size_t infix_ptr = 0; infix_ptr < num_infix_tokens; infix_ptr++) {
        if (infix[infix_ptr].token_type == IDENTIFIER) {
            tokens[postfix_tokens++] = infix[infix_ptr];
        } else if (infix[infix_ptr].token_type == OPERATION) {
            while (
                stack_ptr > 0 
                && stack[stack_ptr - 1].token_type == OPERATION 
                && compare_operations(
                    infix[infix_ptr].token[0], stack[stack_ptr - 1].token[0]
                ) <= 0
            ) {
                tokens[postfix_tokens++] = stack[--stack_ptr];
            }
            stack[stack_ptr++] = infix[infix_ptr];
        } else if (infix[infix_ptr].token_type == PARENTHESIS) {
            if (infix[infix_ptr].token[0] == '(') {
                stack[stack_ptr++] = infix[infix_ptr];
            } else {
                while (stack_ptr > 0 && stack[stack_ptr - 1].token_type != PARENTHESIS) {
                    tokens[postfix_tokens++] = stack[--stack_ptr];
                }
                /* Pop opening parenthesis */
                stack_ptr--;
            }
        }
    }

    while (stack_ptr > 0 && stack[stack_ptr - 1].token_type != PARENTHESIS) {
        tokens[postfix_tokens++] = stack[--stack_ptr];
    }
    *num_tokens = postfix_tokens;
    return true;
}
