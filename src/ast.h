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
	BASE_TYPE_BOOL,
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
	NODE_FUNC_CALL,
	NODE_COMMA,
	NODE_FUNCTION,
	NODE_EXP_SEQ,
	NODE_BRANCH,
	NODE_IF,
	NODE_RETURN,
	NODE_LOGIC_AND,
	NODE_LOGIC_OR,
	NODE_CMP_EQ,
	NODE_CMP_NEQ,
	NODE_CMP_LT,
	NODE_CMP_GT,
	NODE_CMP_LE,
	NODE_CMP_GE,
	NODE_WHILE,
	NODE_BREAK,
	NODE_TRUE,
	NODE_FALSE,
	NODE_NULL,
	NODE_CONTINUE,
	NODE_STRING,
	NODE_CAST
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
	Token *func_name_token;
} NodeFuncCall;

typedef struct
{
	Token *literal_token;
} NodeNumber;

typedef struct
{
	Token *literal_token;
} NodeString;

typedef struct
{
	Token *name_token;
	FuncDescriptor func_descriptor;
} NodeFunction;

typedef struct
{
	TypeDescriptor cast_type;
} NodeCast;

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
		NodeFuncCall func_call;
		NodeFunction function;
		NodeString string;
		NodeCast cast;
	};
};
typedef struct Node Node;

extern bool ast_tokens(Vector *tokens);
extern int type_descriptor_size(TypeDescriptor *descriptor);

#endif