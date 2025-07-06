#ifndef LANGUAGE_H
#define LANGUAGE_H
#include "tokenize.h"

enum LangBaseType
{
	LANG_TYPE_INVALID,
	LANG_TYPE_VOID,
	LANG_TYPE_U8,
	LANG_TYPE_U16,
	LANG_TYPE_I8,
	LANG_TYPE_I16,
};

enum LangOperator
{
	LANG_OP_INVALID,
	LANG_OP_ADD,
	LANG_OP_SUB,
	LANG_OP_MUL,
	LANG_OP_REF,
	LANG_OP_DEREF,
	LANG_OP_ASSIGN,
	LANG_OP_CAST,
};

struct TypeDescriptor
{
	enum LangBaseType base_type;
	int ptr_count;
};

struct BaseTypeInfo
{
	int width_bytes;
	bool is_signed;
	bool is_algebraic;
};

extern void lang_base_type_info(enum LangBaseType type, struct BaseTypeInfo *info);
extern int operator_precedence(enum LangOperator operator);
extern enum LangBaseType lang_base_type_from_token(TokenType token_type);

#endif
