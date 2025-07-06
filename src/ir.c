#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "ir.h"

struct IrContext ir_create_context()
{
	return (struct IrContext)
	{
		.inst_vector = vec_new(struct IrInst *, 10),
		.variables = vec_new(struct IrVar, 10),
		.next_var_number = 1
	};
}

void ir_free_context(struct IrContext *ctx)
{
	for (int i = 0; i < ctx->inst_vector.size; i++)
	{
		free(vec_at(struct IrInst *, &ctx->inst_vector, i));
	}
	vec_free(&ctx->inst_vector);
	vec_free(&ctx->variables);
}

struct IrVar *find_var(struct IrContext *ctx, int var_number)
{
	for (int i = 0; i < ctx->variables.size; i++)
	{
		struct IrVar *var = &vec_at(struct IrVar, &ctx->variables, i);
		if (var->var_number == var_number)
			return var;
	}
	return NULL;
}

void ir_push_inst(struct IrContext *ctx, struct IrInst *inst)
{
	struct IrVar *var = find_var(ctx, inst->dst_var);
	if (var == NULL)
	{
		struct IrVar new_var = (struct IrVar)
		{
			.var_number = inst->dst_var,
			.type = inst->dst_type
		};
		vec_push(struct IrVar, &ctx->variables, &new_var);
	}

	vec_push(struct IrInst *, &ctx->inst_vector, &inst);
	if (ctx->last_instruction == NULL || ctx->first_instruction == NULL) 
	{
		ctx->last_instruction = inst;
		ctx->first_instruction = inst;
		return;
	}
	inst->prev = ctx->last_instruction;
	ctx->last_instruction->next = inst;
	ctx->last_instruction = inst;
}

struct IrInst *ir_push_define(struct IrContext *ctx, enum IrBaseType base_type, uint64_t value)
{
	struct IrInst *inst = malloc(sizeof(struct IrInst));
	*inst = (struct IrInst)
	{
		.type = IRINST_DEFINE,
		.dst_var = ctx->next_var_number++,
		.dst_type = (struct IrTypeDescriptor)
		{
			.base_type = base_type
		},
		.define = (struct IrInstDefine)
		{
			.value = value
		}
	};

	ir_push_inst(ctx, inst);

	return inst;
}

struct IrInst *ir_push_add(struct IrContext *ctx, int lvar, int rvar, int dst_var)
{
	//TODO: Check that lvar and rvar and dst_var exist and are of the same type
	struct IrVar *lvar_definition = find_var(ctx, lvar);
	struct IrVar *rvar_definition = find_var(ctx, rvar);
	assert(lvar_definition != NULL);
	assert(rvar_definition != NULL);
	assert(lvar_definition->type.base_type == rvar_definition->type.base_type);
	if (dst_var != 0)
	{
		struct IrVar *dst_var_definition = find_var(ctx, dst_var);
		assert(dst_var_definition != NULL);
		assert(dst_var_definition->type.base_type == lvar_definition->type.base_type);
	}
	else
	{
		dst_var = ctx->next_var_number++;
	}

	struct IrInst *inst = malloc(sizeof(struct IrInst));
	*inst = (struct IrInst)
	{
		.type = IRINST_ADD,
		.dst_var = dst_var,
		.dst_type = lvar_definition->type,
		.add = (struct IrInstAdd)
		{
			.lvar = lvar,
			.rvar = rvar
		}
	};

	ir_push_inst(ctx, inst);

	return inst;
}

struct IrInst *ir_push_mul(struct IrContext *ctx, int lvar, int rvar, int dst_var)
{
	//TODO: Check that lvar and rvar and dst_var exist and are of the same type
	struct IrVar *lvar_definition = find_var(ctx, lvar);
	struct IrVar *rvar_definition = find_var(ctx, rvar);
	assert(lvar_definition != NULL);
	assert(rvar_definition != NULL);
	assert(lvar_definition->type.base_type == rvar_definition->type.base_type);
	if (dst_var != 0)
	{
		struct IrVar *dst_var_definition = find_var(ctx, dst_var);
		assert(dst_var_definition != NULL);
		assert(dst_var_definition->type.base_type == lvar_definition->type.base_type);
	}
	else
	{
		dst_var = ctx->next_var_number++;
	}

	struct IrInst *inst = malloc(sizeof(struct IrInst));
	*inst = (struct IrInst)
	{
		.type = IRINST_MUL,
		.dst_var = dst_var,
		.dst_type = lvar_definition->type,
		.add = (struct IrInstAdd)
		{
			.lvar = lvar,
			.rvar = rvar
		}
	};

	ir_push_inst(ctx, inst);

	return inst;
}

struct IrInst *ir_push_trunc(struct IrContext *ctx, int src_var, enum IrBaseType dst_type)
{
	//TODO: Check that src exists
	struct IrVar *src_var_definition = find_var(ctx, src_var);
	assert(src_var_definition != NULL);
	int dst_var = ctx->next_var_number++;

	struct IrInst *inst = malloc(sizeof(struct IrInst));
	*inst = (struct IrInst)
	{
		.type = IRINST_TRUNC,
		.dst_var = dst_var,
		.dst_type = dst_type,
		.trunc = (struct IrInstTrunc)
		{
			.src_var = src_var
		}
	};

	ir_push_inst(ctx, inst);

	return inst;

}

struct IrInst *ir_push_extend(struct IrContext *ctx, int src_var, enum IrBaseType dst_type, bool sign_extend)
{
	//TODO: Check that src exists
	struct IrVar *src_var_definition = find_var(ctx, src_var);
	assert(src_var_definition != NULL);
	int dst_var = ctx->next_var_number++;

	struct IrInst *inst = malloc(sizeof(struct IrInst));
	*inst = (struct IrInst)
	{
		.type = IRINST_EXTEND,
		.dst_var = dst_var,
		.dst_type = dst_type,
		.extend = (struct IrInstExtend)
		{
			.src_var = src_var,
			.sign_extend = sign_extend
		}
	};

	ir_push_inst(ctx, inst);

	return inst;
}

struct IrInst *ir_push_copy(struct IrContext *ctx, int src_var, int dst_var)
{
	//TODO: error checking
	struct IrVar *src_definition = find_var(ctx, src_var);
	assert(src_definition != NULL);

	if (dst_var != 0)
	{
		struct IrVar *dst_var_definition = find_var(ctx, dst_var);
		assert(dst_var_definition != NULL);
		assert(dst_var_definition->type.base_type == src_definition->type.base_type);
	}
	else
	{
		dst_var = ctx->next_var_number++;
	}

	struct IrInst *inst = malloc(sizeof(struct IrInst));
	*inst = (struct IrInst)
	{
		.type = IRINST_COPY,
		.dst_var = dst_var,
		.dst_type = src_definition->type,
		.copy = (struct IrInstCopy)
		{
			.src_var = src_var
		}
	};

	ir_push_inst(ctx, inst);

	return inst;
}

const char *get_base_type_str(enum IrBaseType type)
{
	switch (type)
	{
	case IRTYPE_I0:
		return "i0";
	case IRTYPE_I8:
		return "i8";
	case IRTYPE_I16:
		return "i16";
	case IRTYPE_PTR:
		return "ptr";
	}
	return "";
}

void ir_print_context(struct IrContext *ctx)
{
	puts("Printing IR context");
	if (ctx->first_instruction == NULL) return;

	struct IrInst *inst = ctx->first_instruction;

	while (inst != NULL)
	{
		switch (inst->type)
		{
		case IRINST_DEFINE:
			printf("v%i %s = %lli\n", inst->dst_var, get_base_type_str(inst->dst_type.base_type), inst->define.value);
			break;
		case IRINST_ADD:
			printf("v%i %s = add v%i v%i\n", inst->dst_var, get_base_type_str(inst->dst_type.base_type), inst->add.lvar, inst->add.rvar);
			break;
		case IRINST_MUL:
			printf("v%i %s = mul v%i v%i\n", inst->dst_var, get_base_type_str(inst->dst_type.base_type), inst->mul.lvar, inst->mul.rvar);
			break;
		case IRINST_COPY:
			printf("v%i %s = v%i\n", inst->dst_var, get_base_type_str(inst->dst_type.base_type), inst->copy.src_var);
			break;
		case IRINST_TRUNC:
			printf("v%i %s = trunc v%i\n", inst->dst_var, get_base_type_str(inst->dst_type.base_type), inst->trunc.src_var);
			break;
		case IRINST_EXTEND:
			if (inst->extend.sign_extend)
				printf("v%i %s = sextend v%i\n", inst->dst_var, get_base_type_str(inst->dst_type.base_type), inst->copy.src_var);
			else
				printf("v%i %s = uextend v%i\n", inst->dst_var, get_base_type_str(inst->dst_type.base_type), inst->copy.src_var);
			break;
		default:
			printf("Unknown IRINST %i\n", inst->type);
		}

		inst = inst->next;
	}
}
