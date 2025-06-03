#include "ast.h"
#include "tokenize.h"
#include <stdlib.h>
#include <stdio.h>

void pretty_print_tree(Node *node, FILE *file, int depth)
{
	for (int i = 0; i < depth; i++)
	{
		fputc('-', file);
	}
	switch (node->type)
	{
	case NODE_VAR_DECL:
		fprintf(file, "DECL (%s)", node->var_decl.var_name_token->name);
		break;
	case NODE_ADD:
		fputs("ADD", file);
		break;
	case NODE_SUBTRACT:
		fputs("SUB", file);
		break;
	case NODE_MULTIPLY:
		fputs("MUL", file);
		break;
	case NODE_DEREF:
		fputs("DEREF", file);
		break;
	case NODE_REF:
		fputs("REF", file);
		break;
	case NODE_ASSIGN:
		fputs("ASSIGN", file);
		break;
	case NODE_VAR:
		fprintf(file, "VAR (%s)", node->variable.var_name_token->name);
		break;
	case NODE_NUMBER:
		fprintf(file, "NUMBER (%s)", node->number.literal_token->name);
		break;
	case NODE_FUNC_CALL:
		fprintf(file, "CALL (%s)", node->func_call.func_name_token->name);
		break;
	case NODE_COMMA:
		fputs("COMMA", file);
		break;
	case NODE_FUNCTION:
		fprintf(file, "FUNC (%s)", node->function.name_token->name);
		break;
	case NODE_EXP_SEQ:
		fputs("SEQ", file);
		break;
	case NODE_BRANCH:
		fputs("BRANCH", file);
		break;
	case NODE_IF:
		fputs("IF", file);
		break;
	case NODE_RETURN:
		fputs("RETURN", file);
		break;
	case NODE_LOGIC_AND:
		fputs("LOGIC_AND", file);
		break;
	case NODE_LOGIC_OR:
		fputs("LOGIC_OR", file);
		break;
	case NODE_CMP_EQ:
		fputs("CMP_EQ", file);
		break;
	case NODE_CMP_NEQ:
		fputs("CMP_NEQ", file);
		break;
	case NODE_CMP_LT:
		fputs("CMP_LT", file);
		break;
	case NODE_CMP_GT:
		fputs("CMP_GT", file);
		break;
	case NODE_CMP_LE:
		fputs("CMP_LE", file);
		break;
	case NODE_CMP_GE:
		fputs("CMP_GE", file);
		break;
	case NODE_WHILE:
		fputs("WHILE", file);
		break;
	case NODE_BREAK:
		fputs("BREAK", file);
		break;
	case NODE_TRUE:
		fputs("TRUE", file);
		break;
	case NODE_FALSE:
		fputs("FALSE", file);
		break;
	case NODE_NULL:
		fputs("NULL", file);
		break;
	case NODE_CONTINUE:
		fputs("CONTINUE", file);
		break;
	case NODE_STRING:
		fputs("STRING", file);
		break;
	case NODE_CAST:
		fputs("CAST", file);
		break;
	default:
		fputs("ERROR UNKNOWN NODE TYPE", file);
		break;
	}
	fputs("\n", file);

	if (node->left)
		pretty_print_tree(node->left, file, depth + 1);
	if (node->right)
		pretty_print_tree(node->right, file, depth + 1);
}

typedef struct
{
	bool success;
	Node *node;
	int next_index;
} AstBlockResult;
AstBlockResult ast_block(Vector *tokens, int index);

//Determins if the token represents a value
bool token_is_value(Token *token)
{
	switch (token->type)
	{
	case TOKEN_INT:
	case TOKEN_IDENTIFIER:
	case TOKEN_STR_LITERAL:
	case TOKEN_TRUE:
	case TOKEN_FALSE:
		return true;
	}
	return false;
}

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

	switch (token->type)
	{
	case TOKEN_BOOL:
		result.type_descriptor.base_type = BASE_TYPE_BOOL;
		index++;
		break;
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
	default:
		return result;
	}

	if (index >= tokens->size)
		return result;
	token = vec_at(Token*, tokens, index);

	while (token->type == TOKEN_STAR)
	{
		result.type_descriptor.ptr_count++;
		index++;
		if (index >= tokens->size)
			return result;
		token = vec_at(Token*, tokens, index);
	}

	result.next_index = index;
	result.success = true;
	return result;
}

typedef struct 
{
	bool success;
	int next_index;
	FuncDescriptor func_descriptor;
} FunctionParseResult;

static FunctionParseResult parse_function(Vector *tokens, int index)
{
	FunctionParseResult result = {0};
	Token *token = vec_at(Token*, tokens, index);

	TypeDescriptorParseResult return_type_result = parse_type_descriptor(tokens, index);
	if (!return_type_result.success)
		return result;
	if (return_type_result.next_index >= tokens->size)
		return result;
	result.func_descriptor.return_type = return_type_result.type_descriptor;
	index = return_type_result.next_index;
	token = vec_at(Token*, tokens, index);

	if (token->type != TOKEN_IDENTIFIER)
		return result;
	result.func_descriptor.name_token = token;

	index++;
	if (index >= tokens->size)
		return result;
	token = vec_at(Token*, tokens, index);

	if (token->type != TOKEN_OPEN_PAREN)
		return result;

	index++;
	if (index >= tokens->size)
		return result;
	token = vec_at(Token*, tokens, index);

	if (token->type == TOKEN_CLOSE_PAREN)
	{
		index++;
		result.next_index = index;
		result.success = true;
		return result;
	}

	result.func_descriptor.parameters = vec_new(FuncParamDescriptor, 1);
	while (1)
	{
		FuncParamDescriptor func_param = {0};
		TypeDescriptorParseResult param_type_result = parse_type_descriptor(tokens, index);
		if (!param_type_result.success) goto err_cleanup;
		if (param_type_result.next_index >= tokens->size) goto err_cleanup;
		func_param.type = param_type_result.type_descriptor;
		index = param_type_result.next_index;
		token = vec_at(Token*, tokens, index);

		if (token->type != TOKEN_IDENTIFIER) goto err_cleanup;
		func_param.name_token = token;
		index++;
		if (index >= tokens->size) goto err_cleanup;
		token = vec_at(Token*, tokens, index);

		vec_push(FuncParamDescriptor, &result.func_descriptor.parameters, &func_param);

		if (token->type == TOKEN_COMMA)
		{
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_CLOSE_PAREN)
		{
			index++;
			break;
		}
	}

	result.success = true;
	result.next_index = index;
	return result;

	err_cleanup:
	vec_free(&result.func_descriptor.parameters);
	result.success = false;
	return result;
}

void free_tree(Node *tree)
{
	if (tree->left)
		free_tree(tree->left);
	if (tree->right)
		free_tree(tree->right);
	free(tree);
}

int node_precedence(Node *node)
{

	switch (node->type)
	{
	case NODE_COMMA:
		return 0;
	case NODE_ASSIGN:
		return 1;
	case NODE_LOGIC_OR:
		return 3;
	case NODE_LOGIC_AND:
		return 4;
	case NODE_CMP_EQ:
	case NODE_CMP_NEQ:
		return 8;
	case NODE_CMP_LE:
	case NODE_CMP_GE:
	case NODE_CMP_LT:
	case NODE_CMP_GT:
		return 9;
	case NODE_ADD:
	case NODE_SUBTRACT:
		return 11;
	case NODE_MULTIPLY:
		return 12;
	case NODE_REF:
	case NODE_DEREF:
	case NODE_CAST:
		return 13;
	case NODE_FUNC_CALL:
		return 14;
	case NODE_VAR:
	case NODE_NUMBER:
	case NODE_STRING:
	case NODE_VAR_DECL:
		return 1000;

	}
	return 0;
}

//TODO: It's possible for the user to pass in bogus input with parentesis and cause memory leaks
Node *append_tree(Node *tree, Node *leaf)
{
	int leaf_precedence = node_precedence(leaf);

	Node *current_node = tree;
	int current_precedence = 0;
	while ((current_precedence = node_precedence(current_node)) < leaf_precedence && current_node->right && !current_node->paren ||
			//Precedence level 1 operators are evaluated right to left (eg. =, +=, *=, etc.)
			current_precedence | leaf_precedence == 1 && current_node->right)
		current_node = current_node->right;

	if ((current_precedence < leaf_precedence && !current_node->paren) || leaf->paren) 
	{
		if (!current_node->left)
			current_node->left = leaf;
		else
			current_node->right = leaf;
		leaf->up = current_node;
		return tree;
	}

	if (current_node->up)
		current_node->up->right = leaf;
	leaf->up = current_node->up;
	leaf->left = current_node;
	current_node->up = leaf;
	if (current_node == tree)
		return leaf;
	return tree;
}

typedef struct
{
	bool success;
	int next_index;
	Node *node;
} ParseExpressionResult;

ParseExpressionResult parse_expression(Vector *tokens, int index)
{
	Node *tree = NULL;

	while(1)
	{
		Token *token = vec_at(Token*, tokens, index);

		//Check for variable declaration
		TypeDescriptorParseResult parse_type_result = parse_type_descriptor(tokens, index);
		if (parse_type_result.success &&
			parse_type_result.next_index < tokens->size &&
			vec_at(Token*, tokens, parse_type_result.next_index)->type == TOKEN_IDENTIFIER)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_VAR_DECL };
			node->var_decl.var_type = parse_type_result.type_descriptor;
			node->var_decl.var_name_token = vec_at(Token*, tokens, parse_type_result.next_index);
			tree = (tree == NULL ? node : append_tree(tree, node));
			index = parse_type_result.next_index + 1;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		//Check for a cast
		if (token->type == TOKEN_OPEN_PAREN)
		{
			int cast_index = index + 1;
			if (cast_index >= tokens->size) goto not_cast;
			TypeDescriptorParseResult type_result = parse_type_descriptor(tokens, cast_index);
			if (!type_result.success) goto not_cast;
			cast_index = type_result.next_index;
			if (cast_index >= tokens->size) goto not_cast;

			token = vec_at(Token*, tokens, cast_index);
			if (token->type != TOKEN_CLOSE_PAREN) goto not_cast;
			cast_index++;
			if (cast_index >= tokens->size) goto not_cast;

			token = vec_at(Token*, tokens, cast_index);
			if (token->type != TOKEN_OPEN_PAREN && !token_is_value(token)) goto not_cast;

			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_CAST };
			node->cast.cast_type = type_result.type_descriptor;
			tree = (tree == NULL ? node : append_tree(tree, node));
			index = cast_index;
			continue;
		}
		not_cast:

		if (token->type == TOKEN_STR_LITERAL)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_STRING };
			node->string.literal_token = token;
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_CONTINUE)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_CONTINUE };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_TRUE)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_TRUE };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_FALSE)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_FALSE };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_NULL)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_NULL };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_CMP_EQ)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_CMP_EQ };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_CMP_NEQ)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_CMP_NEQ };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_CMP_LT)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_CMP_LT };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_CMP_GT)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_CMP_GT };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_CMP_LE)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_CMP_LE };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_CMP_GE)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_CMP_GE };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_LOGIC_AND)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_LOGIC_AND };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_LOGIC_OR)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_LOGIC_OR };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_PLUS)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_ADD };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_MINUS)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_SUBTRACT };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_AMP)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_REF };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_EQUAL)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_ASSIGN };
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		//Check for function call
		if (token->type == TOKEN_IDENTIFIER &&
			index + 1 <= tokens->size &&
			vec_at(Token*, tokens, index + 1)->type == TOKEN_OPEN_PAREN)
		{
			index += 2;
			if (index >= tokens->size) goto err_cleanup;
			Token *name_token = token;

			token = vec_at(Token*, tokens, index);
			if (token->type == TOKEN_CLOSE_PAREN)
			{
				index++;
				if (index >= tokens->size) goto err_cleanup;
				Node *node = malloc(sizeof(Node));
				*node = (Node){ .type = NODE_FUNC_CALL, .paren = true };
				node->func_call.func_name_token = name_token;
				tree = (tree == NULL ? node : append_tree(tree, node));
				continue;
			}

			Node *param_tree = NULL;

			while (1)
			{
				ParseExpressionResult recurse_result = parse_expression(tokens, index);
				if (!recurse_result.success) goto err_param_cleanup;
				if (!recurse_result.node) goto err_param_cleanup;
				param_tree = (param_tree == NULL ? recurse_result.node : append_tree(param_tree, recurse_result.node));

				index = recurse_result.next_index;
				if (index >= tokens->size) goto err_param_cleanup;
				token = vec_at(Token*, tokens, index);

				if (token->type == TOKEN_COMMA)
				{
					Node *node = malloc(sizeof(Node));
					*node = (Node){ .type = NODE_COMMA };
					param_tree = (param_tree == NULL ? node : append_tree(param_tree, node));
					index++;
					if (index >= tokens->size) goto err_param_cleanup;
					continue;
				}

				break;
			}

			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_FUNC_CALL, .paren = true };
			node->func_call.func_name_token = name_token;
			node->left = param_tree;
			param_tree->up = node;
			tree = (tree == NULL ? node : append_tree(tree, node));
			continue;

			err_param_cleanup:
			if (param_tree)
				free_tree(param_tree);
			goto err_cleanup;
		}

		if (token->type == TOKEN_IDENTIFIER)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_VAR };
			node->variable.var_name_token = token;
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_INT)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_NUMBER };
			node->number.literal_token = token;
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_STAR)
		{
			Node *node = malloc(sizeof(Node));

			if (index > 0 &&
				vec_at(Token*, tokens, index - 1)->type == TOKEN_IDENTIFIER ||
				vec_at(Token*, tokens, index - 1)->type == TOKEN_INT ||
				vec_at(Token*, tokens, index - 1)->type == TOKEN_CLOSE_PAREN)
			{
				*node = (Node){ .type = NODE_MULTIPLY };
			}
			else
			{
				*node = (Node){ .type = NODE_DEREF };
			}
			tree = (tree == NULL ? node : append_tree(tree, node));
			index++;
			if (index >= tokens->size) goto err_cleanup;
			continue;
		}

		if (token->type == TOKEN_OPEN_PAREN)
		{
			index++;
			if (index >= tokens->size) goto err_cleanup;
			ParseExpressionResult recurse_result = parse_expression(tokens, index);
			if (!recurse_result.success) goto err_cleanup;
			index = recurse_result.next_index;
			if (index >= tokens->size) goto err_cleanup;
			tree = (tree == NULL ? recurse_result.node : append_tree(tree, recurse_result.node));
			continue;
		}

		if (token->type == TOKEN_CLOSE_PAREN)
		{
			if (tree)
				tree->paren = true;
			index++;
			break;
		}

		if (token->type == TOKEN_COMMA)
		{
			break;
		}

		if (token->type == TOKEN_SEMICOLON)
		{
			index++;
			break;
		}

		goto err_cleanup;
	}

	return (ParseExpressionResult){ .success = true, .next_index = index, .node = tree };

	err_cleanup:
	if (tree) free_tree(tree);
	return (ParseExpressionResult){ .success = false };
}

typedef struct
{
	bool success;
	Node *node;
	int next_index;
} AstReturnStatement;

AstReturnStatement parse_return_statement(Vector *tokens, int index)
{
	Token *token = vec_at(Token*, tokens, index);
	if (token->type != TOKEN_RETURN) return (AstReturnStatement){ .success = false };
	index++;
	if (index >= tokens->size) return (AstReturnStatement){ .success = false };

	ParseExpressionResult exp_result = parse_expression(tokens, index);
	if (!exp_result.success) return (AstReturnStatement){ .success = false };

	Node *node = malloc(sizeof(Node));
	*node = (Node){ .type = NODE_RETURN };
	node->left = exp_result.node;
	exp_result.node->up = node;

	return (AstReturnStatement){ .success = true, .node = node, .next_index = exp_result.next_index };
}

typedef struct
{
	bool success;
	Node *node;
	int next_index;
} AstIfResult;

AstIfResult parse_if_statement(Vector *tokens, int index)
{
	Node *exp_node = NULL;
	Node *if_body_node = NULL;
	Node *else_body_node = NULL;

	Token *token = vec_at(Token*, tokens, index);
	if (token->type != TOKEN_IF) return (AstIfResult){ .success = false };
	index++;
	if (index >= tokens->size) return (AstIfResult){ .success = false };
	token = vec_at(Token*, tokens, index);

	if (token->type != TOKEN_OPEN_PAREN) return (AstIfResult){ .success = false };
	index++;
	if (index >= tokens->size) return (AstIfResult){ .success = false };
	token = vec_at(Token*, tokens, index);

	ParseExpressionResult exp_result = parse_expression(tokens, index);
	if (!exp_result.success) goto err_cleanup;
	exp_node = exp_result.node;
	index = exp_result.next_index;
	if (index >= tokens->size) goto err_cleanup;

	AstBlockResult if_body_result = ast_block(tokens, index);
	if (!if_body_result.success) goto err_cleanup;
	if_body_node = if_body_result.node;
	index = if_body_result.next_index;
	if (index >= tokens->size) goto err_cleanup;

	token = vec_at(Token*, tokens, index);
	if (token->type != TOKEN_ELSE) goto place_node;
	index++;
	if (index >= tokens->size) goto err_cleanup;
	AstBlockResult else_body_result = ast_block(tokens, index);
	if (!else_body_result.success) goto err_cleanup;
	else_body_node = else_body_result.node;
	index = else_body_result.next_index;
	if (index >= tokens->size) goto err_cleanup;

	place_node:
	Node *if_node = malloc(sizeof(Node));
	*if_node = (Node){ .type = NODE_IF };
	if_node->left = exp_node;
	exp_node->up = if_node;

	Node *branch_node = malloc(sizeof(Node));
	*branch_node = (Node){ .type = NODE_BRANCH };
	if_node->right = branch_node;
	branch_node->up = if_node;
	branch_node->left = if_body_node;
	if_body_node->up = branch_node;
	if (else_body_node)
	{
		else_body_node->up = branch_node;
		branch_node->right = else_body_node;
	}

	return (AstIfResult) { .success = true, .node = if_node, .next_index = index };

	err_cleanup:
	if (exp_node)
		free_tree(exp_node);
	if (if_body_node)
		free_tree(if_body_node);
	if (else_body_node)
		free_tree(else_body_node);
	return (AstIfResult){ .success = false };
}

typedef struct
{
	bool success;
	Node *node;
	int next_index;
} AstWhileResult;

AstWhileResult parse_while_statement(Vector *tokens, int index)
{
	Node *exp_node = NULL;
	Node *while_body = NULL;

	Token *token = vec_at(Token*, tokens, index);
	if (token->type != TOKEN_WHILE) return (AstWhileResult){ .success = false };
	index++;
	if (index >= tokens->size) return (AstWhileResult){ .success = false };
	token = vec_at(Token*, tokens, index);

	if (token->type != TOKEN_OPEN_PAREN) return (AstWhileResult){ .success = false };
	index++;
	if (index >= tokens->size) return (AstWhileResult){ .success = false };
	token = vec_at(Token*, tokens, index);

	ParseExpressionResult exp_result = parse_expression(tokens, index);
	if (!exp_result.success) goto err_cleanup;
	exp_node = exp_result.node;
	index = exp_result.next_index;
	if (index >= tokens->size) goto err_cleanup;

	AstBlockResult while_body_result = ast_block(tokens, index);
	if (!while_body_result.success) goto err_cleanup;
	while_body = while_body_result.node;
	index = while_body_result.next_index;
	if (index >= tokens->size) goto err_cleanup;

	Node *while_node = malloc(sizeof(Node));
	*while_node = (Node){ .type = NODE_WHILE };
	while_node->left = exp_node;
	exp_node->up = while_node;
	while_node->right = while_body;
	while_body->up = while_node;
	return (AstWhileResult) { .success = true, .node = while_node, .next_index = index };

	err_cleanup:
	if (exp_node)
		free_tree(exp_node);
	if (while_body)
		free_tree(while_body);
	return (AstWhileResult){0};
}

AstBlockResult ast_block(Vector *tokens, int index)
{
	AstBlockResult block_result = {0};

	while (vec_at(Token*, tokens, index)->type == TOKEN_OPEN_BRACE)
	{
		index++;
		if (index >= tokens->size) goto err_cleanup;
	}

	while (1)
	{
		Node *placed_node = NULL;
		int next_index = 0;

		AstReturnStatement return_result = parse_return_statement(tokens, index);
		if (return_result.success)
		{
			placed_node = return_result.node;
			next_index = return_result.next_index;
			goto place_node;
		}

		AstIfResult if_result = parse_if_statement(tokens, index);
		if (if_result.success)
		{
			placed_node = if_result.node;
			next_index = if_result.next_index;
			goto place_node;
		}

		AstWhileResult while_result = parse_while_statement(tokens, index);
		if (while_result.success)
		{
			placed_node = while_result.node;
			next_index = while_result.next_index;
			goto place_node;
		}

		if (vec_at(Token*, tokens, index)->type == TOKEN_BREAK)
		{
			placed_node = malloc(sizeof(Node));
			*placed_node = (Node) { .type = NODE_BREAK };
			next_index = index + 1;
			goto place_node;
		}

		ParseExpressionResult exp_result = parse_expression(tokens, index);
		placed_node = exp_result.node;
		next_index = exp_result.next_index;
		if (!exp_result.success) goto err_cleanup;

		place_node:
		if (!block_result.node)
		{
			block_result.node = placed_node;
			goto node_placed;
		}

		if (block_result.node->type != NODE_EXP_SEQ || block_result.node->right)
		{
			Node *node = malloc(sizeof(Node));
			*node = (Node){ .type = NODE_EXP_SEQ };
			node->left = block_result.node;
			block_result.node->up = node;
			block_result.node = node;
			node->right = placed_node;
			placed_node->up = node;
			goto node_placed;
		}

		block_result.node->right = placed_node;
		placed_node->up = block_result.node;

		node_placed:
		index = next_index;
		if (index >= tokens->size) goto err_cleanup;
		Token *token = vec_at(Token*, tokens, index);

		if (token->type == TOKEN_CLOSE_BRACE)
		{
			block_result.success = true;
			block_result.next_index = index + 1;
			return block_result;
		}
	}

	err_cleanup:
	if (block_result.node)
		free_tree(block_result.node);
	return (AstBlockResult){ .success = false };
}

typedef struct
{
	bool success;
	Node *node;
	int next_index;
} AstFuncResult;

AstFuncResult ast_function(Vector *tokens, int index)
{
	FunctionParseResult func_result = parse_function(tokens, 0);
	if (!func_result.success) return (AstFuncResult){ .success = false };
	index = func_result.next_index;
	if (index >= tokens->size) return (AstFuncResult){ .success = false };

	Token *token = vec_at(Token*, tokens, index);
	if (token->type != TOKEN_OPEN_BRACE) return (AstFuncResult){ .success = false };

	index++;
	if (index >= tokens->size) return (AstFuncResult){ .success = false };

	AstBlockResult block_result = ast_block(tokens, index);
	if (!block_result.success) return (AstFuncResult){ .success = false };

	Node *node = malloc(sizeof(Node));
	*node = (Node){ .type = NODE_FUNCTION };
	node->function.name_token = func_result.func_descriptor.name_token;
	node->function.func_descriptor = func_result.func_descriptor;
	node->left = block_result.node;
	block_result.node->up = node;

	return (AstFuncResult){ .success = true, .node = node, .next_index = block_result.next_index };
}

bool ast_tokens(Vector *tokens)
{
	AstFuncResult result = ast_function(tokens, 0);

	if (result.success)
	{
		pretty_print_tree(result.node, stdout, 0);
	}
	else
	{
		puts("Failed to parse function");
	}
}