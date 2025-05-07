#ifndef AST_H
#define AST_H
#include "list.h"
#include "tokenize.h"

typedef enum
{
	NODE_INVALID,
} AstNodeType;

typedef enum 
{
	BASE_TYPE_INVALID,
	BASE_TYPE_VOID,
	BASE_TYPE_U16,
	BASE_TYPE_S16,
	BASE_TYPE_FUNC,
} BaseType;

struct FuncTypeDescriptor;
typedef struct
{
	BaseType base_type;
	int ptr_count;
	union
	{
		struct FuncTypeDescriptor *func_descriptor;
	} descriptors;
} TypeDescriptor;

typedef struct
{
	Token *name_token;
	TypeDescriptor type;
} FuncParamDescriptor;

typedef struct
{
	TypeDescriptor return_type;
	Vector parameters;
} FuncTypeDescriptor;

extern void ast_tokens(Vector *tokens);

#endif