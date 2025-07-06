#include "language.h"

enum LangBaseType parse_base_type(Token *token)
{
	if (!strcmp(token->name, "void")) return LANG_TYPE_VOID;
	if (!strcmp(token->name, "u8")) return LANG_TYPE_U8;
	if (!strcmp(token->name, "u16")) return LANG_TYPE_U16;
	if (!strcmp(token->name, "i8")) return LANG_TYPE_I8;
	if (!strcmp(token->name, "i16")) return LANG_TYPE_I16;
	return LANG_TYPE_INVALID;
}

int operator_precedence(enum LangOperator operator)
{
	switch (operator)
	{
	case LANG_OP_ADD:
		return 4;
	case LANG_OP_SUB:
		return 4;
	case LANG_OP_MUL:
		return 3;
	case LANG_OP_REF:
		return 2;
	case LANG_OP_DEREF:
		return 2;
	case LANG_OP_ASSIGN:
		return 14;
	}

	return 0;
}

void lang_base_type_info(enum LangBaseType type, struct BaseTypeInfo *info)
{
	switch (type)
	{
	case LANG_TYPE_VOID:
		info->width_bytes = 0;
		info->is_signed = false;
		info->is_algebraic = false;
		break;
	case LANG_TYPE_U8:
		info->width_bytes = 1;
		info->is_signed = false;
		info->is_algebraic = true;
		break;
	case LANG_TYPE_U16:
		info->width_bytes = 2;
		info->is_signed = false;
		info->is_algebraic = true;
		break;
	case LANG_TYPE_I8:
		info->width_bytes = 1;
		info->is_signed = true;
		info->is_algebraic = true;
		break;
	case LANG_TYPE_I16:
		info->width_bytes = 2;
		info->is_signed = true;
		info->is_algebraic = true;
		break;
	}

}

enum LangBaseType lang_base_type_from_token(TokenType token_type)
{
	if (token_type == TOKEN_VOID) return LANG_TYPE_VOID;
	if (token_type == TOKEN_U16) return LANG_TYPE_U16;
	if (token_type == TOKEN_U8) return LANG_TYPE_U8;
	if (token_type == TOKEN_I16) return LANG_TYPE_I16;
	if (token_type == TOKEN_I8) return LANG_TYPE_I8;
	return LANG_TYPE_INVALID;
}