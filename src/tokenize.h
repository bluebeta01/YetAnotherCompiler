#ifndef TOKENIZE_H
#define TOKENIZE_H

#include "list.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
	TOKEN_INVALID,
	TOKEN_INT,
	TOKEN_IDENTIFIER,
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_STAR,
	TOKEN_OPEN_PAREN,
	TOKEN_CLOSE_PAREN,
	TOKEN_ARROW,
	TOKEN_S16,
	TOKEN_U16,
	TOKEN_VOID,
	TOKEN_COLON,
	TOKEN_OPEN_BRACE,
	TOKEN_CLOSE_BRACE,
	TOKEN_SEMICOLON,
	TOKEN_EQUAL,
	TOKEN_LESS_THAN,
	TOKEN_GREATER_THAN,
	TOKEN_AMP,
	TOKEN_FOR,
	TOKEN_OPEN_BRACKET,
	TOKEN_CLOSE_BRACKET,
	TOKEN_STRUCT,
	TOKEN_STATIC,
	TOKEN_DOT,
	TOKEN_COMMA,
	TOKEN_ARRAY_DECL,
} TokenType;

typedef struct
{
	TokenType type;
	int line;
	int column;
	const char *name;
	uint64_t int_literal;
	bool int_signed;
} Token;

extern bool tokenize_file(const char *filepath, Vector *tokens);

#endif
