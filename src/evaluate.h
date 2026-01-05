#pragma once

#define TOKEN_SIZE      20
#define MAX_TOKENS      255


enum TokenType {
    IDENTIFIER='i', OPERATION='o', PARENTHESES='p'
};


struct ExpressionToken {
    char                    token[TOKEN_SIZE];
    char                    token_type;
    struct ExpressionToken  *next;
};


struct ExpressionToken *tokenize_expression(char []);
bool transform_and_check_expression(struct ExpressionToken *);
struct ExpressionToken *to_postfix_notation(struct ExpressionToken *);

int compare_operations(char, char);
bool is_arithmetic_operation(const char);
bool is_parentheses(const char);
