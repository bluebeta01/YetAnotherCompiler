#ifndef AST_H
#define AST_H
#include "list.h"
#include "tokenize.h"

typedef enum 
{
	BASE_TYPE_INVALID,
	BASE_TYPE_VOID,
	BASE_TYPE_U16,
	BASE_TYPE_S16,
} BaseType;

typedef struct
{
	BaseType base_type;
	int ptr_count;
} TypeDescriptor;

typedef struct
{
	Token *name_token;
	TypeDescriptor type;
} FuncParamDescriptor;

typedef struct
{
	Token *name_token;
	TypeDescriptor return_type;
	Vector parameters;
} FuncDescriptor;

typedef enum
{
	NODE_INVALID,
	NODE_VAR_DECL,
	NODE_ADD,
	NODE_SUBTRACT,
	NODE_MULTIPLY,
	NODE_DEREF,
	NODE_REF,
	NODE_ASSIGN,
	NODE_VAR,
	NODE_NUMBER,
} NodeType;

typedef struct
{
	TypeDescriptor var_type;
	Token *var_name_token;
} NodeVarDecl;

typedef struct
{
	Token *var_name_token;
} NodeVar;

typedef struct
{
	Token *literal_token;
} NodeNumber;

struct Node
{
	NodeType type;
	struct Node *left;
	struct Node *right;
	struct Node *up;
	bool paren;
	union
	{
		NodeVarDecl var_decl;
		NodeVar variable;
		NodeNumber number;
	};
};
typedef struct Node Node;

extern bool ast_tokens(Vector *tokens);

#endif