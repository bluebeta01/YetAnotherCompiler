#include "ast.h"
#include "tokenize.h"
#include <stdlib.h>

typedef struct 
{
	bool success;
	int next_index;
	TypeDescriptor type_descriptor;
} TypeDescriptorParseResult;

static TypeDescriptorParseResult parse_type_descriptor(Vector *tokens, int index)
{
	TypeDescriptorParseResult result = {0};
	Token *token = vec_at(Token*, tokens, index);

	while (token->type == TOKEN_STAR)
	{
		result.type_descriptor.ptr_count++;
		index++;
		if (index >= tokens->size)
			return result;
		token = vec_at(Token*, tokens, index);
	}

	bool err = false;
	FuncTypeDescriptor *func = NULL;
	switch (token->type)
	{
	case TOKEN_U16:
		result.type_descriptor.base_type = BASE_TYPE_U16;
		index++;
		break;
	case TOKEN_S16:
		result.type_descriptor.base_type = BASE_TYPE_S16;
		index++;
		break;
	case TOKEN_VOID:
		result.type_descriptor.base_type = BASE_TYPE_VOID;
		index++;
		break;
	case TOKEN_OPEN_PAREN:
		{
			result.type_descriptor.base_type = BASE_TYPE_FUNC;
			func = malloc(sizeof(FuncTypeDescriptor));
			*func = (FuncTypeDescriptor){0};
			func->parameters = vec_new(FuncParamDescriptor, 1);

			index++;
			if (index >= tokens->size)
			{
				err = true;
				goto err_cleanup;
			}
			token = vec_at(Token*, tokens, index);
			//This function has parameters
			if (token->type != TOKEN_CLOSE_PAREN)
			{
				while (1)
				{
					//TODO: Parse the parameters
					if (token->type != TOKEN_IDENTIFIER)
					{
						err = true;
						goto err_cleanup;
					}
					FuncParamDescriptor param = { .name_token = token };
					index++;
					if (index >= tokens->size)
					{
						err = true;
						goto err_cleanup;
					}
					token = vec_at(Token*, tokens, index);
					if (token->type != TOKEN_COLON)
					{
						err = true;
						goto err_cleanup;
					}
					index++;
					if (index >= tokens->size)
					{
						err = true;
						goto err_cleanup;
					}
					token = vec_at(Token*, tokens, index);

					TypeDescriptorParseResult paramTypeParseResult = parse_type_descriptor(tokens, index);
					if (!paramTypeParseResult.success)
					{
						err = true;
						goto err_cleanup;
					}
					param.type = paramTypeParseResult.type_descriptor;
					vec_push(FuncParamDescriptor, &func->parameters, &param);
					index = paramTypeParseResult.next_index;

					if (index >= tokens->size)
					{
						err = true;
						goto err_cleanup;
					}
					token = vec_at(Token*, tokens, index);
					if (token->type == TOKEN_CLOSE_PAREN) break;
					if (token->type == TOKEN_COMMA)
					{
						index++;
						if (index >= tokens->size)
						{
							err = true;
							goto err_cleanup;
						}
						token = vec_at(Token*, tokens, index);
					}
				}
			}

			//We should now be at the closing paren
			index++;
			if (index >= tokens->size)
			{
				err = true;
				goto err_cleanup;
			}
			token = vec_at(Token*, tokens, index);
			if (token->type != TOKEN_ARROW)
			{
				err = true;
				goto err_cleanup;
			}
			index++;
			if (index >= tokens->size)
			{
				err = true;
				goto err_cleanup;
			}

			//Parse the return type
			TypeDescriptorParseResult returnTypeResult = parse_type_descriptor(tokens, index);
			if (!returnTypeResult.success)
			{
				err = true;
				goto err_cleanup;
			}
			index = returnTypeResult.next_index;
			func->return_type = returnTypeResult.type_descriptor;

			result.type_descriptor.descriptors.func_descriptor = func;
			break;
		}
	default:
		return result;
	}

	err_cleanup:
	if (err)
	{
		if (func)
		{
			vec_free(&func->parameters);
			free(func);
			func = NULL;
		}
		return result;
	}

	result.next_index = index;
	result.success = true;
	return result;
}

typedef struct 
{
	bool success;
	int next_index;
} FunctionParseResult;

static FunctionParseResult parse_function(Vector *tokens, int index)
{
	FunctionParseResult result = {0};
	Token *token = vec_at(Token*, tokens, index);

	if (token->type != TOKEN_IDENTIFIER)
		return result;

	index++;
	if (index >= tokens->size)
		return result;
	token = vec_at(Token*, tokens, index);

	if (token->type != TOKEN_COLON)
		return result;

	index++;
	if (index >= tokens->size)
		return result;
	token = vec_at(Token*, tokens, index);

	TypeDescriptorParseResult type_result = parse_type_descriptor(tokens, index);
	if (!type_result.success)
		return result;
}

void ast_tokens(Vector *tokens)
{
	TypeDescriptorParseResult result = parse_type_descriptor(tokens, 0);
}