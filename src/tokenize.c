#include "tokenize.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

//If str1 starts with str2, returns the length of str2. Otherwise returns 0.
static int starts_with(const char *str1, const char *str2)
{
	int len = strlen(str2);
	if (!strncmp(str1, str2, len))
		return len;
	return 0;
}

//If str1 starts with str2 and then contains a non-identifier character, return the length of str2
//Otherwise return 0
static int is_word_token(const char *str1, const char *str2)
{
	int len = starts_with(str1, str2);
	if (len == 0) return 0;
	str1 += len;
	if (!isalnum(*str1) && *str1 != '_')
		return len;
	return 0;
}

typedef struct
{
	TokenType type;
	int token_length;
} TokenLookupResult;

static TokenLookupResult token_type_lookup(const char *search_str, TokenType previous_type)
{
	int length = 0;

	length = is_word_token(search_str, "continue");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_CONTINUE, .token_length = length };
	length = is_word_token(search_str, "null");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_NULL, .token_length = length };
	length = is_word_token(search_str, "false");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_FALSE, .token_length = length };
	length = is_word_token(search_str, "true");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_TRUE, .token_length = length };
	length = is_word_token(search_str, "break");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_BREAK, .token_length = length };
	length = is_word_token(search_str, "while");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_WHILE, .token_length = length };
	length = is_word_token(search_str, "else");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_ELSE, .token_length = length };
	length = is_word_token(search_str, "if");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_IF, .token_length = length };
	length = is_word_token(search_str, "return");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_RETURN, .token_length = length };
	length = is_word_token(search_str, "void");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_VOID, .token_length = length };
	length = is_word_token(search_str, "for");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_FOR, .token_length = length };
	length = is_word_token(search_str, "struct");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_STRUCT, .token_length = length };
	length = is_word_token(search_str, "static");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_STATIC, .token_length = length };
	length = is_word_token(search_str, "@array");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_ARRAY_DECL, .token_length = length };

	length = starts_with(search_str, "||");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_LOGIC_OR, .token_length = length };
	length = starts_with(search_str, "&&");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_LOGIC_AND, .token_length = length };
	length = starts_with(search_str, ">=");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_CMP_GE, .token_length = length };
	length = starts_with(search_str, "<=");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_CMP_LE, .token_length = length };
	length = starts_with(search_str, "!=");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_CMP_NEQ, .token_length = length };
	length = starts_with(search_str, "==");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_CMP_EQ, .token_length = length };
	length = starts_with(search_str, "->");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_ARROW, .token_length = length };
	length = starts_with(search_str, "i16");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_I16, .token_length = length };
	length = starts_with(search_str, "u16");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_U16, .token_length = length };
	length = starts_with(search_str, "i8");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_I8, .token_length = length };
	length = starts_with(search_str, "u8");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_U8, .token_length = length };
	length = starts_with(search_str, "bool");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_BOOL, .token_length = length };
	length = starts_with(search_str, "+");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_PLUS, .token_length = length };
	length = starts_with(search_str, "-");
	//If the previous token was a minus, this one must be part of a number literal or is invalid
	if (length > 0 && previous_type != TOKEN_MINUS) return (TokenLookupResult){ .type = TOKEN_MINUS, .token_length = length };
	length = starts_with(search_str, "*");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_STAR, .token_length = length };
	length = starts_with(search_str, "(");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_OPEN_PAREN, .token_length = length };
	length = starts_with(search_str, ")");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_CLOSE_PAREN, .token_length = length };
	length = starts_with(search_str, ":");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_COLON, .token_length = length };
	length = starts_with(search_str, "{");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_OPEN_BRACE, .token_length = length };
	length = starts_with(search_str, "}");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_CLOSE_BRACE, .token_length = length };
	length = starts_with(search_str, ";");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_SEMICOLON, .token_length = length };
	length = starts_with(search_str, "=");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_EQUAL, .token_length = length };
	length = starts_with(search_str, "<");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_CMP_LT, .token_length = length };
	length = starts_with(search_str, ">");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_CMP_GT, .token_length = length };
	length = starts_with(search_str, "&");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_AMP, .token_length = length };
	length = starts_with(search_str, "[");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_OPEN_BRACKET, .token_length = length };
	length = starts_with(search_str, "]");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_CLOSE_BRACKET, .token_length = length };
	length = starts_with(search_str, ".");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_DOT, .token_length = length };
	length = starts_with(search_str, ",");
	if (length > 0) return (TokenLookupResult){ .type = TOKEN_COMMA, .token_length = length };

	return (TokenLookupResult){ .type = TOKEN_INVALID };
}

typedef struct 
{
	bool success;
	bool is_negative;
	uint32_t int_literal;
	int str_length;
} ParseIntLiteralResult;

static Token *create_token(const char *token_str, int token_str_len, TokenType type, uint64_t int_literal, bool is_negative)
{
	char *name_str = malloc(token_str_len + 1);
	memcpy(name_str, token_str, token_str_len);
	name_str[token_str_len] = 0;

	Token *token = malloc(sizeof(Token));
	*token = (Token)
	{
		.type = type,
		.name = name_str,
		.int_literal = int_literal,
		.is_negative = is_negative
	};

	return token;
}

static ParseIntLiteralResult parse_int_literal(const char *input)
{
	ParseIntLiteralResult result = {0};
	if (*input == '-')
	{
		result.is_negative = true;
		result.str_length++;
		input++;
	}
	if (!isdigit(*input))
		return result;

	char *end;
	result.int_literal = (uint64_t)strtol(input, &end, 0);
	result.success = true;
	result.str_length = end - input;
	if (result.is_negative)
		result.str_length++;

	return result;
}

typedef struct 
{
	bool success;
	int str_length;
} ParseIdentifierResult;

static ParseIdentifierResult parse_identifier(const char *input)
{
	if (*input != '_' && !isalpha(*input))
		return (ParseIdentifierResult){0};

	ParseIdentifierResult result = { .str_length = 1 };
	input++;

	while (isalnum(*input) || *input == '_')
	{
		result.str_length++;
		input++;
	}

	result.success = true;
	return result;
}

typedef struct 
{
	bool success;
	int str_length;
} ParseStrLiteralResult;

static ParseStrLiteralResult parse_str_literal(const char *input)
{
	const char *start = input;
	if (*input != '"') return (ParseStrLiteralResult){0};
	input++;
	if (*input == 0) return (ParseStrLiteralResult){0};

	while (1)
	{
		if (*input == '"' && *(input - 1) != '\\')
			return (ParseStrLiteralResult) { .success = true, .str_length = input - start - 1 };
		if (*input == 0)
			return (ParseStrLiteralResult){0};
		input++;
	}

	return (ParseStrLiteralResult){0};
}

static bool take_token(const char *filedata, Vector *tokens)
{
	TokenType previous_type = TOKEN_INVALID;
	int line = 1;
	int col = 1;

	while (1)
	{
		while (isspace(*filedata))
		{
			if (*filedata == '\n')
			{
				line++;
				col = 0;
			}

			col++;
			filedata++;
		}

		TokenLookupResult result = token_type_lookup(filedata, previous_type);
		if (result.type != TOKEN_INVALID)
		{
			previous_type = result.type;
			Token *token = create_token(filedata, result.token_length, result.type, 0, false);
			token->line = line;
			token->column = col;
			col += strlen(token->name);
			vec_push(Token*, tokens, &token);
			filedata += result.token_length;
			continue;
		}

		ParseStrLiteralResult str_result = parse_str_literal(filedata);
		if (str_result.success)
		{
			previous_type = TOKEN_STR_LITERAL;
			Token *token = create_token(filedata + 1, str_result.str_length, TOKEN_STR_LITERAL, 0, false);
			token->line = line;
			token->column = col;
			col += strlen(token->name);
			vec_push(Token*, tokens, &token);
			filedata += str_result.str_length + 2;
			continue;
		}

		ParseIntLiteralResult int_result = parse_int_literal(filedata);
		if (int_result.success)
		{
			previous_type = TOKEN_INT;
			Token *token = create_token(filedata, int_result.str_length, TOKEN_INT, int_result.int_literal, int_result.is_negative);
			token->line = line;
			token->column = col;
			col += strlen(token->name);
			vec_push(Token*, tokens, &token);
			filedata += int_result.str_length;
			continue;
		}

		ParseIdentifierResult id_result = parse_identifier(filedata);
		if (id_result.success)
		{
			previous_type = TOKEN_IDENTIFIER;
			Token *token = create_token(filedata, id_result.str_length, TOKEN_IDENTIFIER, 0, false);
			token->line = line;
			token->column = col;
			col += strlen(token->name);
			vec_push(Token*, tokens, &token);
			filedata += id_result.str_length;
			continue;
		}

		return false;
	}

	return true;
}

bool tokenize_file(const char *filepath, Vector *tokens)
{
	FILE *file = fopen(filepath, "rb");
	if (!file)
	{
		printf("Failed to open %s", filepath);
		return false;
	}

	fseek(file, 0, SEEK_END);
	long file_length = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (file_length == 0)
	{
		fclose(file);
		return;
	}

	char *buffer = malloc(sizeof(char) * file_length + 1);
	long bytes_read = fread(buffer, sizeof(char), file_length, file);
	buffer[file_length] = 0;

	if (bytes_read != file_length)
	{
		fclose(file);
		free(buffer);
		printf("Failed to read file %s", filepath);
		return false;
	}

	fclose(file);

	return take_token(buffer, tokens);
}
