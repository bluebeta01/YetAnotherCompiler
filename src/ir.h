/*
	Variables are listed v0 through v[n] and are integers 8-32 bits.
	Variables are always immutable
*/

#ifndef IR_H
#define IR_H
#include "list.h"
#include <stdint.h>
#include <stdbool.h>

#define PTR_WIDTH_BYTES 2
#define PTR_IR_TYPE IRTYPE_I16

enum IrBaseType
{
	IRTYPE_I0,
	IRTYPE_I8,
	IRTYPE_I16,
	IRTYPE_PTR,
};

struct IrTypeDescriptor
{
	enum IrBaseType base_type;
};

enum IrInstType
{
	IRINST_DEFINE,
	IRINST_ADD,
	IRINST_MUL,
	IRINST_COPY,
	IRINST_EXTEND,
	IRINST_TRUNC,
};

struct IrInstDefine
{
	uint64_t value;
};

struct IrInstAdd
{
	int lvar;
	int rvar;
};

struct IrInstMul
{
	int lvar;
	int rvar;
};

struct IrInstCopy
{
	int src_var;
};

struct IrInstExtend
{
	int src_var;
	bool sign_extend;
};

struct IrInstTrunc
{
	int src_var;
};

struct IrInst
{
	enum IrInstType type;
	int dst_var;
	struct IrTypeDescriptor dst_type;
	struct IrInst *next;
	struct IrInst *prev;
	union
	{
		struct IrInstDefine define;
		struct IrInstAdd add;
		struct IrInstMul mul;
		struct IrInstCopy copy;
		struct IrInstExtend extend;
		struct IrInstTrunc trunc;
	};
};

struct IrVar
{
	int var_number;
	struct IrTypeDescriptor type;
};

struct IrContext
{
	Vector inst_vector;
	Vector variables;
	struct IrInst *last_instruction;
	struct IrInst *first_instruction;
	int next_var_number;
};

extern struct IrContext ir_create_context();
extern void ir_free_context(struct IrContext *ctx);
extern void ir_print_context(struct IrContext *ctx);
extern struct IrInst *ir_push_define(struct IrContext *ctx, enum IrBaseType base_type, uint64_t value);
extern struct IrInst *ir_push_add(struct IrContext *ctx, int lvar, int rvar, int dst_var);
extern struct IrInst *ir_push_mul(struct IrContext *ctx, int lvar, int rvar, int dst_var);
extern struct IrInst *ir_push_copy(struct IrContext *ctx, int src_var, int dst_var);
extern struct IrInst *ir_push_extend(struct IrContext *ctx, int src_var, enum IrBaseType dst_type, bool sign_extend);
extern struct IrInst *ir_push_trunc(struct IrContext *ctx, int src_var, enum IrBaseType dst_type);

#endif
