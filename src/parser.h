#ifndef PARSER_H
#define PARSER_H
#include "list.h"
#include "tokenize.h"

typedef enum ParseResult
{
	PR_OK = 0,
	PR_UNEXPECTED_TOKEN,
	PR_EOF,
	PR_INTERNAL_ERROR,
	PR_SYNTAX
} ParseResult;


enum AstNodeType
{
	AST_NODE_INVALID,
	AST_NODE_SEQ,
	AST_NODE_VALUE,
	AST_NODE_VAR_DECL,

	AST_NODE_MUL,
	AST_NODE_ADD,
	AST_NODE_SUB,
	AST_NODE_ASSIGN,
};

enum BaseType
{
	BASE_TYPE_UNKNOWN,
	BASE_TYPE_FUNC,
	BASE_TYPE_VOID,
	BASE_TYPE_I8,
	BASE_TYPE_U8,
	BASE_TYPE_I16,
	BASE_TYPE_U16,
};

struct FuncTypeDescriptor;

struct TypeDescriptor
{
	enum BaseType base;
	int ptr_count;
	Token *name_token;
	struct FuncTypeDescriptor *func_type_descriptor;
};

struct FuncTypeDescriptor
{
	struct TypeDescriptor return_type;
	Vector parameter_declarations;
};

struct Declaration
{
	struct TypeDescriptor type;
	Token *token;
};

struct AstVarDecl
{
	struct Declaration declaration;
};

struct AstNode
{
	enum AstNodeType type;
	Token *token;
	struct AstNode *left;
	struct AstNode *right;
	struct AstNode *parent;
	struct TypeDescriptor type_descriptor;
	bool paren;
	union
	{
		struct AstVarDecl *var_decl;
	};
};

struct CodeBlock
{
	struct AstNode *tree;
};

struct FuncDescriptor
{
	struct Declaration declaration;
	struct CodeBlock code_block;
};

struct ParserContext
{
	Vector defined_types;
	Vector functions;
};

struct ParserContext parser_init();
void parser_deinit(struct ParserContext *ctx);
ParseResult parse_file(struct ParserContext *ctx, Vector *tokens);
void print_parser_error();

#endif