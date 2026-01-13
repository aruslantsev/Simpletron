#pragma once

#include <stddef.h>


#define TOKEN_SIZE      20
#define MAX_TOKENS      255


enum TokenType {
    IDENTIFIER='i', OPERATION='o', PARENTHESES='p'
};


struct ExpressionToken {
    char                    token[TOKEN_SIZE];
    char                    token_type;
};


bool tokenize_expression(char [], struct ExpressionToken [], size_t *);

int compare_operations(char, char);
bool is_arithmetic_operation(const char);
bool is_parentheses(const char);
