#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


struct ExpressionToken *tokenize_expression(char expression[]) {
    /* Add trailing space. Last token will be saved automatically */
    strcat(expression, " ");
#ifdef DEBUG_EVAL
    printf("Tokenizing expression '%s'\n", expression);
#endif

    struct ExpressionToken *start = NULL, *current_token = NULL;
    char current_identifier[TOKEN_SIZE];
    size_t token_idx = 0;
    const char *c = expression;
    while (*c != '\0') {
        /* Operation, parentheses, blank, end of line == token separator */
        if (
            is_parentheses(*c)
            || is_arithmetic_operation(*c)
            || isblank(*c)
            || *c == '\n'
        ) {
            /* Something was read during previous step */
            if (token_idx > 0) {
                /* Save token */
                current_identifier[token_idx] = '\0';
                struct ExpressionToken *token = (struct ExpressionToken *) malloc(
                    sizeof(struct ExpressionToken)
                );
                strcpy(token->token, current_identifier);
                token->next = NULL;
#ifdef DEBUG_EVAL
                printf("Infix notation: adding token '%s'\n", token->token);
#endif
                token->token_type = IDENTIFIER;
                token_idx = 0;
                if (start == NULL) {
                    start = token;
                    current_token = start;
                } else {
                    current_token->next = token;
                    current_token = current_token->next;
                }
            }
            if (is_parentheses(*c) || is_arithmetic_operation(*c)) {
                /* Save parentheses and operations: read single symbol to token */
                struct ExpressionToken *token = (struct ExpressionToken *) malloc(
                    sizeof(struct ExpressionToken)
                );
                token->token[0] = *c;
                token->token[1] = '\0';
                if (is_arithmetic_operation(*c)) {
                    token->token_type = OPERATION;
                } else {
                    token->token_type = PARENTHESES;
                }
                token->next = NULL;
#ifdef DEBUG_EVAL
                printf("Infix notation: adding token '%s'\n", token->token);
#endif
                if (start == NULL) {
                    start = token;
                    current_token = start;
                } else {
                    current_token->next = token;
                    current_token = current_token->next;
                }
            } else if (!isblank(*c) && *c != '\n') {
                printf("Unknown symbol '%c'\n", *c);
                exit(1);
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
                current_identifier[token_idx++] = *c;
                c++;
            }
        }
    }
    if (transform_and_check_expression(start)) {
        return to_postfix_notation(start);
    }
    return NULL;
}


bool transform_and_check_expression(struct ExpressionToken *expression_start) {
#ifdef DEBUG_EVAL
    puts("Checking and transforming expression");
#endif
    int parentheses_count = 0;
    struct ExpressionToken *current_token = expression_start;
#ifdef DEBUG_EVAL
    puts("Got infix expression");
    while (current_token != NULL) {
        printf("'%s' ", current_token->token);
        current_token = current_token->next;
    }
    puts("");
    current_token = expression_start;
#endif
    int token_cnt = 0;
    /* Leading unary operator */
    if (current_token != NULL && current_token->token_type == OPERATION) {
        if (current_token->token[0] == '+' || current_token->token[0] == '-') {
#ifdef DEBUG_EVAL
            puts("Leading unary plus or minus found");
#endif
            current_token->token_type = IDENTIFIER;
            current_token->token[1] = '1';
            current_token->token[2] = '\0';
            struct ExpressionToken *tmp_token = (struct ExpressionToken *) malloc(
                sizeof(struct ExpressionToken)
            );
            tmp_token->token_type = OPERATION;
            strcpy(tmp_token->token, "*");
            tmp_token->next = current_token->next;
            current_token->next = tmp_token;
        } else {
            return false;
        }
    }
    /* Other tokens */
    while (current_token != NULL) {
#ifdef DEBUG_EVAL
        printf(
            "Got token '%s' with type '%c'. Token number %d\n",
            current_token->token, current_token->token_type, token_cnt
        );
#endif
        if (token_cnt > MAX_TOKENS)
            return false;

        /* Two identifiers without operation between them */
        if (current_token->token_type == IDENTIFIER) {
            if (
               current_token->next != NULL
               && current_token->next->token_type == IDENTIFIER
           ) {
                puts("Error: Found two tokens without operation between them.");
                return false;
           }
        } else if (
            current_token->token_type == PARENTHESES
            || current_token->token_type == OPERATION
        ) {
            /* Count parentheses */
            if (current_token->token_type == PARENTHESES) {
                if (current_token->token[0] == '(') {
                    parentheses_count++;
                    if (
                        current_token->next != NULL
                        && current_token->next->token_type == PARENTHESES
                        && current_token->next->token[0] == ')'
                    ) {
                        puts("Error: Parentheses without expression inside.");
                        return false;
                    }
                } else {
                    parentheses_count--;
                    if (parentheses_count < 0) {
                        puts(
                            "Error: Found closing parentheses without "
                            "corresponding opening one."
                        );
                        return false;
                    }
                }
            }

            /* Three operations without identifiers or two trailing operations
            * or opening parentheses and two operations */
            if (
                (
                    current_token->token_type == OPERATION
                    || (
                        current_token->token_type == PARENTHESES
                        && current_token->token[0] == '('
                    )
                )
                && current_token->next != NULL
                && current_token->next->token_type == OPERATION
            ) {
                if (
                    current_token->next->next == NULL
                    || current_token->next->next->token_type == OPERATION
                ) {
                    puts(
                        "Error: three consecutive operators or opening "
                        "parentheses and two operators found."
                    );
                    return false;
                }

                /* Two operations without identifiers or opening parentheses
                 * and operation: second can be only unary + or unary - */
                if (
                    current_token->next->token[0] == '+'
                    || current_token->next->token[0] == '-'
                ) {
#ifdef DEBUG_EVAL
                    puts("Unary plus or minus found");
#endif
                    // transform unary - or unary + to (-1 * ...) or (+1 * ...)
                    current_token->next->token[1] = '1';
                    current_token->next->token[2] = '\0';
                    current_token->next->token_type = IDENTIFIER;
                    struct ExpressionToken *tmp_token = (struct ExpressionToken *) malloc(
                        sizeof(struct ExpressionToken)
                    );
                    tmp_token->token_type = OPERATION;
                    strcpy(tmp_token->token, "*");
                    tmp_token->next = current_token->next->next;
                    current_token->next->next = tmp_token;
                } else return false;
            }
        } else {
            puts("Unknown token");
            return false;
        }
        current_token = current_token->next;
        token_cnt++;
    }
    return parentheses_count == 0;
}

struct ExpressionToken *to_postfix_notation(struct ExpressionToken *expression_start) {
#ifdef DEBUG_EVAL
    puts("Converting to postfix notation.");
#endif
    struct ExpressionToken *current_infix_token = expression_start;
    struct ExpressionToken *postfix_expression = NULL;
    struct ExpressionToken *current_postfix_token = NULL;
    struct ExpressionToken *tmp_token = NULL;
    struct ExpressionToken *operations_stack[MAX_TOKENS];
    size_t stack_ptr = 0;
    int token_cnt = 0;

    while (current_infix_token != NULL) {
#ifdef DEBUG_EVAL
        printf(
            "Token '%s', type '%c'\n",
            current_infix_token->token, current_infix_token->token_type
        );
        token_cnt++;
        if (token_cnt > MAX_TOKENS) {
            exit(1);
        }
#endif
        if (current_infix_token->token_type == IDENTIFIER) {
#ifdef DEBUG_EVAL
            puts("Placing token to output queue.");
#endif
            /* Add identifier to queue */
            if (postfix_expression == NULL) {
                postfix_expression = current_infix_token;
                current_postfix_token = current_infix_token;
                current_infix_token = current_infix_token->next;
                current_postfix_token->next = NULL;
            } else {
                current_postfix_token->next = current_infix_token;
                current_postfix_token = current_postfix_token->next;
                current_infix_token = current_infix_token->next;
                current_postfix_token->next = NULL;
            }
        } else if (current_infix_token->token_type == OPERATION) {
            while (
                stack_ptr > 0
                && operations_stack[stack_ptr - 1]->token_type == OPERATION
                && compare_operations(
                    current_infix_token->token[0], operations_stack[stack_ptr - 1]->token[0]
                ) <= 0
            ) {
#ifdef DEBUG_EVAL
                printf(
                    "Moving token '%s', type '%c' from stack to output queue.\n",
                    operations_stack[stack_ptr - 1]->token,
                    operations_stack[stack_ptr - 1]->token_type
                );
#endif
                current_postfix_token->next = operations_stack[--stack_ptr];
                current_postfix_token = current_postfix_token->next;
                current_postfix_token->next = NULL;
            }
#ifdef DEBUG_EVAL
            printf(
                "Placing token '%s', type '%c' to stack\n",
                current_infix_token->token, current_infix_token->token_type
            );
#endif
            operations_stack[stack_ptr++] = current_infix_token;
            current_infix_token = current_infix_token->next;
            operations_stack[stack_ptr - 1]->next = NULL;
        } else if (
            current_infix_token->token_type == PARENTHESES
            && current_infix_token->token[0] == '('
        ) {
#ifdef DEBUG_EVAL
            puts("To stack (opening parentheses).");
#endif
            operations_stack[stack_ptr++] = current_infix_token;
            current_infix_token = current_infix_token->next;
            operations_stack[stack_ptr - 1]->next = NULL;
        } else if (
            current_infix_token->token_type == PARENTHESES
            && current_infix_token->token[0] == ')'
        ) {
#ifdef DEBUG_EVAL
            puts("Closing parentheses.");
#endif
            while (
                stack_ptr > 0 && operations_stack[stack_ptr - 1]->token_type != PARENTHESES
            ) {
#ifdef DEBUG_EVAL
                printf(
                    "Moving token '%s', type '%c' from stack to output queue.\n",
                    operations_stack[stack_ptr - 1]->token,
                    operations_stack[stack_ptr - 1]->token_type
                );
#endif
                current_postfix_token->next = operations_stack[--stack_ptr];
                current_postfix_token = current_postfix_token->next;
                current_postfix_token->next = NULL;
            }
            puts("Pop opening parentheses.");
            tmp_token = operations_stack[--stack_ptr];
            free(tmp_token);
            tmp_token = current_infix_token;
            current_infix_token = current_infix_token->next;
            free(tmp_token);
        }
    }
#ifdef DEBUG_EVAL
    if (stack_ptr > 0) {
        puts("Stack is not empty.");
    }
#endif
    while (stack_ptr > 0) {
#ifdef DEBUG_EVAL
        printf(
            "Moving token '%s', type '%c' from stack to output queue.\n",
            operations_stack[stack_ptr - 1]->token,
            operations_stack[stack_ptr - 1]->token_type
        );
#endif
        current_postfix_token->next = operations_stack[--stack_ptr];
        current_postfix_token = current_postfix_token->next;
        current_postfix_token->next = NULL;
    }
#ifdef DEBUG_EVAL
    token_cnt = 0;
    current_postfix_token = postfix_expression;
    puts("Postfix expression");
    while (current_postfix_token != NULL) {
        printf("%s ", current_postfix_token->token);
        current_postfix_token = current_postfix_token->next;
        token_cnt++;
        if (token_cnt > MAX_TOKENS) {
            exit(1);
        }
    }
    puts("");
#endif
    return postfix_expression;
}
