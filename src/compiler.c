#include "compiler.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#define ERRMSG_SIZE 100

enum ValueLocation
{
	VAL_LOC_TOKEN,
	VAL_LOC_STACK,
	VAL_LOC_VARIABLE,
	VAL_LOC_DECL,
};

struct TypedValue
{
	struct TypeDescriptor type;
	enum ValueLocation location;
	Token *token;
	int ir_var_number;
};

struct TypeInfo
{
	int width_bytes;
	bool is_signed;
	bool is_algebraic;
};

struct Variable
{
	Token *name_token;
	struct TypeDescriptor type;
	int ir_var_number;
};

struct Variable *find_variable(struct CompilerContext *ctx, const char *name)
{
	int count = ctx->variables.size;
	for (int i = 0; i < count; i++)
	{
		struct Variable *v = &vec_at(struct Variable, &ctx->variables, i);
		if (!strcmp(name, v->name_token->name)) return v;
	}
	return NULL;
}

char errmsg[ERRMSG_SIZE];
struct TypedValue value_stack[50];
int value_stack_size = 0;
int value_stack_min = 0;
enum LangOperator operator_stack[50];
int operator_stack_size = 0;
int operator_stack_min = 0;

void set_compiler_error(const char *msg, Token *token)
{
	snprintf(errmsg, ERRMSG_SIZE, "(%i, %i): %s", token->line, token->column, msg);
}

void print_compiler_error()
{
	printf("Failed to compile.\n%s\n", errmsg);
}

enum LangOperator pop_operator()
{
	if (operator_stack_size == 0 || operator_stack_size == operator_stack_min) return LANG_OP_INVALID;
	operator_stack_size--;
	return operator_stack[operator_stack_size];
}

enum LangOperator peek_operator()
{
	if (operator_stack_size == 0 || operator_stack_size == operator_stack_min) return LANG_OP_INVALID;
	return operator_stack[operator_stack_size - 1];
}

void push_operator(enum LangOperator operator)
{
	assert(operator_stack_size < 50);
	operator_stack[operator_stack_size] = operator;
	operator_stack_size++;
}

struct TypedValue pop_value()
{
	if (value_stack_size == 0 || value_stack_size == value_stack_min) return value_stack[0];
	value_stack_size--;
	return value_stack[value_stack_size];
}

void push_value(struct TypedValue *value)
{
	assert(value_stack_size < 50);
	value_stack[value_stack_size] = *value;
	value_stack_size++;
}

bool should_evaluate(enum LangOperator next_operator)
{
	enum LangOperator last_operator = peek_operator();
	if (last_operator == LANG_OP_INVALID) return false;
	if (operator_precedence(last_operator) <= operator_precedence(next_operator)) return true;
	return false;
}

void type_info(struct TypeDescriptor *td, struct TypeInfo *info)
{
	if (td->ptr_count > 0)
	{
		info->width_bytes = PTR_WIDTH_BYTES;
		info->is_signed = false;
		info->is_algebraic = true;
		return;
	}
	switch (td->base_type)
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

enum LangBaseType int_literal_base_type(Token *token)
{
	if (token->is_negative)
	{
		if (token->int_literal < 128) return LANG_TYPE_I8;
		if (token->int_literal < 32768) return LANG_TYPE_I16;
	}

	if (token->int_literal < 256) return LANG_TYPE_U8;
	if (token->int_literal < 65536) return LANG_TYPE_U16;

	return LANG_TYPE_INVALID;
}

enum IrBaseType convert_type_descriptor(struct TypeDescriptor *descriptor)
{
	if (descriptor->ptr_count > 0) return PTR_IR_TYPE;
	if (descriptor->base_type == LANG_TYPE_U8 || descriptor->base_type == LANG_TYPE_I8) return IRTYPE_I8;
	if (descriptor->base_type == LANG_TYPE_U16 || descriptor->base_type == LANG_TYPE_I16) return IRTYPE_I16;

	return IRTYPE_I0;
}

void define_ir_number(struct CompilerContext *ctx, struct TypedValue *value)
{
	if (value->location == VAL_LOC_TOKEN)
	{
		enum IrBaseType ir_type = convert_type_descriptor(&value->type);
		struct IrInst *define = ir_push_define(&ctx->ir_context, ir_type, value->token->int_literal);
		value->ir_var_number = define->dst_var;
	}
	if (value->location == VAL_LOC_DECL)
	{
		enum IrBaseType ir_type = convert_type_descriptor(&value->type);
		struct IrInst *define = ir_push_define(&ctx->ir_context, ir_type, value->token->int_literal);
		value->ir_var_number = define->dst_var;
	}
}

//Returns true if the integer literal in the token will fit in the desired type
bool int_can_fit(Token *int_token, enum LangBaseType type)
{
	switch (type)
	{
	case LANG_TYPE_U8:
		if (int_token->is_negative) return false;
		if (int_token->int_literal > 255) return false;
		return true;
	case LANG_TYPE_U16:
		if (int_token->is_negative) return false;
		if (int_token->int_literal > 65535) return false;
		return true;
	case LANG_TYPE_I8:
		if (int_token->is_negative)
		{
			if (int_token->int_literal > 128) return false;
		}
		else
		{
			if (int_token->int_literal > 127) return false;
		}
		return true;
	case LANG_TYPE_I16:
		if (int_token->is_negative)
		{
			if (int_token->int_literal > 32768) return false;
		}
		else
		{
			if (int_token->int_literal > 65535) return false;
		}
		return true;
	default:
		return false;
	}
}

//Attempts to implicity cast the value to the type td. Does nothing if value is already of type td
bool implicit_cast(struct CompilerContext *ctx, struct TypedValue *value, struct TypeDescriptor *td, Token *current_token)
{
	if (value->type.base_type == td->base_type && value->type.ptr_count == td->ptr_count) return true;

	if (td->ptr_count > 0 || value->type.ptr_count > 0)
	{
		set_compiler_error("Cannot implictly cast pointer", current_token);
		return false;
	}

	struct TypeInfo value_type_info = {0};
	type_info(&value->type, &value_type_info);

	if (td->base_type == LANG_TYPE_U8)
	{
		if (value->location == VAL_LOC_TOKEN)
		{
			if (!int_can_fit(value->token, LANG_TYPE_U8))
			{
				set_compiler_error("Implicit cast of integer literal would result in data loss", current_token);
				return false;
			}
			value->type.base_type = LANG_TYPE_U8;
			return true;
		}
		set_compiler_error("Implicit conversion failed", current_token);
		return false;
	}

	if (td->base_type == LANG_TYPE_I8)
	{
		if (value->location == VAL_LOC_TOKEN)
		{
			if (!int_can_fit(value->token, LANG_TYPE_I8))
			{
				set_compiler_error("Implicit cast of integer literal would result in data loss", current_token);
				return false;
			}
			value->type.base_type = LANG_TYPE_I8;
			return true;
		}
		set_compiler_error("Implicit conversion failed", current_token);
		return false;
	}

	if (td->base_type == LANG_TYPE_U16)
	{
		if (value->location == VAL_LOC_TOKEN)
		{
			if (!int_can_fit(value->token, LANG_TYPE_U16))
			{
				set_compiler_error("Implicit cast of integer literal would result in data loss", current_token);
				return false;
			}
			value->type.base_type = LANG_TYPE_U16;
			return true;
		}

		if (value_type_info.is_signed)
		{
			set_compiler_error("Cannot implictly convert a signed value to an unsigned value", current_token);
			return false;
		}
		if (value_type_info.width_bytes > 2)
		{
			set_compiler_error("Cannot implictly convert integer to a smaller type", current_token);
			return false;
		}

		struct IrInst *inst = ir_push_extend(ctx, value->ir_var_number, IRTYPE_I16, false);
		value->type.base_type = LANG_TYPE_U16;
		value->ir_var_number = inst->dst_var;
		return true;
	}

	if (td->base_type == LANG_TYPE_I16)
	{
		if (value->location == VAL_LOC_TOKEN)
		{
			if (!int_can_fit(value->token, LANG_TYPE_I16))
			{
				set_compiler_error("Implicit cast of integer literal would result in data loss", current_token);
				return false;
			}
			value->type.base_type = LANG_TYPE_I16;
			return true;
		}

		if (!value_type_info.is_signed)
		{
			set_compiler_error("Cannot implictly convert a signed value to an unsigned value", current_token);
			return false;
		}
		if (value_type_info.width_bytes > 2)
		{
			set_compiler_error("Cannot implictly convert integer to a smaller type", current_token);
			return false;
		}

		struct IrInst *inst = ir_push_extend(ctx, value->ir_var_number, IRTYPE_I16, true);
		value->type.base_type = LANG_TYPE_I16;
		value->ir_var_number = inst->dst_var;
		return true;
	}

	set_compiler_error("Implicit conversion failed", current_token);
	return false;
}

bool upgrade_ints(struct CompilerContext *ctx, struct TypedValue *v1, struct TypedValue *v2, Token *current_token)
{
	//The smaller type is upgraded to the larger type.
	//Signed and unsigned values cannot be upgraded to each other.
	struct BaseTypeInfo v1_info = {0};
	struct BaseTypeInfo v2_info = {0};
	type_info(&v1->type, &v1_info);
	type_info(&v2->type, &v2_info);

	if (!v1_info.is_algebraic || !v2_info.is_algebraic)
	{
		set_compiler_error("Cannot perform arithemtic on non-algebraic types", current_token);
		return false;
	}

	if (v1_info.width_bytes > v2_info.width_bytes || v2->location == VAL_LOC_TOKEN)
	{
		bool r = implicit_cast(ctx, v2, &v1->type, current_token);
		if (!r) return false;
		return true;
	}

	bool r = implicit_cast(ctx, v1, &v2->type, current_token);
	if (!r) return false;
	return true;
}

void compile_add(struct CompilerContext *ctx, struct TypedValue *v1, struct TypedValue *v2, Token *current_token)
{
	//TODO: error checking
	bool r = upgrade_ints(ctx, v1, v2, current_token);
	if (!r)
	{
		print_compiler_error();
		return;
	}

	define_ir_number(ctx, v1);
	define_ir_number(ctx, v2);
	struct IrInst *add = ir_push_add(&ctx->ir_context, v1->ir_var_number, v2->ir_var_number, 0);

	push_value(&(struct TypedValue)
	{
		.type = v2->type,
		.location = VAL_LOC_STACK,
		.ir_var_number = add->dst_var
	});
}

void compile_mul(struct CompilerContext *ctx, struct TypedValue *v1, struct TypedValue *v2, Token *current_token)
{
	//TODO: error checking
	bool r = upgrade_ints(ctx, v1, v2, current_token);
	if (!r)
	{
		print_compiler_error();
		return;
	}

	define_ir_number(ctx, v1);
	define_ir_number(ctx, v2);
	struct IrInst *mul = ir_push_mul(&ctx->ir_context, v1->ir_var_number, v2->ir_var_number, 0);

	push_value(&(struct TypedValue)
	{
		.type = v2->type,
		.location = VAL_LOC_STACK,
		.ir_var_number = mul->dst_var
	});
}

void compile_assign(struct CompilerContext *ctx, struct TypedValue *v1, struct TypedValue *v2, Token *current_token)
{
	//TODO: error checking
	struct Variable *lvar = NULL;
	if (v1->location == VAL_LOC_DECL)
	{
		struct Variable variable = (struct Variable)
		{
			.name_token = v1->token,
			.type = v1->type,
			.ir_var_number = v1->ir_var_number
		};
		vec_push(struct Variable, &ctx->variables, &variable);
		lvar = &vec_last(struct Variable, &ctx->variables);
	}

	bool r = implicit_cast(ctx, v2, &v1->type, current_token);
	if (!r)
	{
		print_compiler_error();
		return;
	}

	if (v1->location == VAL_LOC_DECL)
	{
		define_ir_number(ctx, v2);
		if (v2->location == VAL_LOC_VARIABLE)
		{
			struct IrInst *copy = ir_push_copy(ctx, v2->ir_var_number, 0);
			lvar->ir_var_number = copy->dst_var;
			return;
		}
		lvar->ir_var_number = v2->ir_var_number;
		return;
	}

	define_ir_number(ctx, v1);
	define_ir_number(ctx, v2);
	struct IrInst *copy = ir_push_copy(ctx, v2->ir_var_number, v1->ir_var_number);
}

void compile_cast(struct CompilerContext *ctx, Token *current_token)
{
	struct TypedValue value = pop_value();
	struct TypeDescriptor td = pop_value().type;

	if (value.type.ptr_count > 0 && td.ptr_count > 0)
	{
		value.type.ptr_count = td.ptr_count;
		value.type.base_type = td.base_type;
		return true;
	}

	struct TypeInfo value_info = {0};
	struct TypeInfo td_info = {0};
	type_info(&value.type, &value_info);
	type_info(&td, &td_info);

	if (!value_info.is_algebraic || !td_info.is_algebraic)
	{
		set_compiler_error("Cannot case non-algebraic types", current_token);
		print_compiler_error();
		return;
	}

	if (value.location == VAL_LOC_TOKEN)
	{
		//TODO: We should check that the integer literal in the token can actually be cast to a ptr without its value changing
		value.type.base_type = td.base_type;
		value.type.ptr_count = td.ptr_count;
		push_value(&value);
		return;
	}

	if (value_info.width_bytes == td_info.width_bytes)
	{
		value.type.base_type = td.base_type;
		value.type.ptr_count = td.ptr_count;
		push_value(&value);
		return;
	}

	enum IrBaseType new_ir_type =  convert_type_descriptor(&td);
	if (td_info.width_bytes < value_info.width_bytes)
	{
		struct IrInst *trunc = ir_push_trunc(ctx, value.ir_var_number, new_ir_type);
		value.ir_var_number = trunc->dst_var;
		value.type.base_type = td.base_type;
		value.type.ptr_count = td.ptr_count;
		push_value(&value);
		return;
	}

	struct IrInst *extend = ir_push_extend(ctx, value.ir_var_number, new_ir_type, value_info.is_signed);
	value.ir_var_number = extend->dst_var;
	value.type.base_type = td.base_type;
	value.type.ptr_count = td.ptr_count;
	push_value(&value);
	return;
}

bool evaluate(struct CompilerContext *ctx, Token *current_token)
{
	enum LangOperator operator = pop_operator();
	if (operator == LANG_OP_ADD)
	{
		struct TypedValue v2 = pop_value();
		struct TypedValue v1 = pop_value();
		compile_add(ctx, &v1, &v2, current_token);
		return true;
	}
	if (operator == LANG_OP_MUL)
	{
		struct TypedValue v2 = pop_value();
		struct TypedValue v1 = pop_value();
		compile_mul(ctx, &v1, &v2, current_token);
		return true;
	}
	if (operator == LANG_OP_ASSIGN)
	{
		struct TypedValue v2 = pop_value();
		struct TypedValue v1 = pop_value();
		compile_assign(ctx, &v1, &v2, current_token);
		return true;
	}
	if (operator == LANG_OP_CAST)
	{
		compile_cast(ctx, current_token);
		return true;
	}
	return false;
}

bool parse_type_descriptor(Token **tokens, int index, int token_count, int *next_index, struct TypeDescriptor *td)
{
	Token *token = tokens[index];
	enum LangBaseType base_type = lang_base_type_from_token(token->type);
	if (base_type == LANG_TYPE_INVALID) return false;

	index++;
	if (index >= token_count) return false;
	token = tokens[index];

	int ptr_count = 0;
	while (token->type == TOKEN_STAR)
	{
		ptr_count++;
		index++;
		if (index >= token_count) return false;
		token = tokens[index];
	}

	*td = (struct TypeDescriptor)
	{
		.base_type = base_type,
		.ptr_count = ptr_count
	};
	*next_index = index;
	return true;
}

bool push_var_decl(Token **tokens, int index, int token_count, int *next_index)
{
	int new_idx = 0;
	struct TypeDescriptor td;
	bool r = parse_type_descriptor(tokens, index, token_count, &new_idx, &td);
	if (!r) return r;

	index = new_idx;
	if (index >= token_count) return false;
	Token *token = tokens[index];

	if (token->type != TOKEN_IDENTIFIER) return false;

	push_value(&(struct TypedValue)
	{
		.type = td,
		.location = VAL_LOC_DECL,
		.token = token
	});

	*next_index = index + 1;
	return true;
}

//Parses a cast. ex. (i16 **)
bool parse_cast(struct CompilerContext *ctx, Token **tokens, int index, int token_count, int *next_index, struct TypeDescriptor *td)
{
	if (tokens[index]->type != TOKEN_OPEN_PAREN) return false;
	index++;
	if (index >= token_count) return false;
	bool r = parse_type_descriptor(tokens, index, token_count, next_index, td);
	if (!r) return false;
	index = *next_index;
	if (index >= token_count) return false;
	if (tokens[index]->type != TOKEN_CLOSE_PAREN) return false;
	*next_index = index + 1;
	return true;
}

bool compile_expression(struct CompilerContext *ctx, Token **tokens, int index, int token_count, int *next_index, bool start)
{
	bool can_deref = true;

	if (start)
	{
		int new_idx = 0;
		bool r = push_var_decl(tokens, index, token_count, &new_idx);
		if (r)
		{
			index = new_idx;
		}
	}

	while (1)
	{
		if (index >= token_count) return false;
		Token *token = tokens[index];
		if (token->type == TOKEN_OPEN_PAREN)
		{
			int new_idx = 0;
			struct TypeDescriptor td = {0};
			bool r = parse_cast(ctx, tokens, index, token_count, &new_idx, &td);
			if (r)
			{
				push_value(&(struct TypedValue)
				{
					.type = td,
					.token = token
				});
				push_operator(LANG_OP_CAST);

				index = new_idx;
				can_deref = true;
				continue;
			}
		}
		if (token->type == TOKEN_OPEN_PAREN)
		{
			int new_index = 0;
			int old_value_stack_min = value_stack_min;
			int old_op_stack_min = operator_stack_min;
			value_stack_min = value_stack_size;
			operator_stack_min = operator_stack_size;
			bool r = compile_expression(ctx, tokens, index + 1, token_count, &new_index, false);
			if (!r) return r;
			index = new_index;
			value_stack_min = old_value_stack_min;
			operator_stack_min = old_op_stack_min;
			can_deref = false;
			continue;
		}
		if (token->type == TOKEN_CLOSE_PAREN || token->type == TOKEN_SEMICOLON)
		{
			while (evaluate(ctx, token));
			*next_index = index + 1;
			return true;
		}
		if (token->type == TOKEN_INT)
		{
			push_value(&(struct TypedValue)
			{
				.type = (struct TypeDescriptor)
				{
					.base_type = int_literal_base_type(token)
				},
				.location = VAL_LOC_TOKEN,
				.token = token
			});
			index++;
			can_deref = false;
			continue;
		}
		if (token->type == TOKEN_PLUS)
		{
			enum LangOperator next_operator = LANG_OP_ADD;
			while (should_evaluate(next_operator))
			{
				bool r = evaluate(ctx, token);
				if (!r) return r;
			}
			push_operator(next_operator);
			index++;
			can_deref = true;
			continue;
		}
		if (token->type == TOKEN_EQUAL)
		{
			enum LangOperator next_operator = LANG_OP_ASSIGN;
			while (should_evaluate(next_operator))
			{
				bool r = evaluate(ctx, token);
				if (!r) return r;
			}
			push_operator(next_operator);
			index++;
			can_deref = true;
			continue;
		}
		if (token->type == TOKEN_STAR && !can_deref)
		{
			enum LangOperator next_operator = LANG_OP_MUL;
			while (should_evaluate(next_operator))
			{
				bool r = evaluate(ctx, token);
				if (!r) return r;
			}
			push_operator(LANG_OP_MUL);
			index++;
			can_deref = true;
			continue;
		}
		if (token->type == TOKEN_IDENTIFIER)
		{
			struct Variable *var = find_variable(ctx, token->name);
			if (!var)
			{
				set_compiler_error("Undeclared identifier", token);
				print_compiler_error();
				return false;
			}
			struct TypedValue value = (struct TypedValue)
			{
				.type = var->type,
				.location = VAL_LOC_VARIABLE,
				.token = var->name_token,
				.ir_var_number = var->ir_var_number
			};
			push_value(&value);
			index++;
			can_deref = false;
			continue;
		}

		set_compiler_error("Unexepcted token", token);
		print_compiler_error();
		return false;
	}
}

bool compile_tokens(struct CompilerContext *ctx, Token **tokens, int index, int token_count)
{
	int next_index = 0;
	bool r = true;
	while (r)
	{
		r = compile_expression(ctx, tokens, next_index, token_count, &next_index, true);
		if (!r) return false;
		if (next_index >= token_count) break;
	}

	ir_print_context(&ctx->ir_context);
}

struct CompilerContext compiler_create_context()
{
	return (struct CompilerContext)
	{
		.ir_context = ir_create_context(),
		.variables = vec_new(struct Variable, 10)
	};
}
