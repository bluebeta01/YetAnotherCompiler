#ifndef COMPILER_H
#define COMPILER_H
#include "language.h"
#include "ir.h"

struct CompilerContext
{
	struct IrContext ir_context;
	Vector variables;
};

extern struct CompilerContext compiler_create_context();
extern bool compile_tokens(struct CompilerContext *ctx, Token **tokens, int index, int token_count);

#endif