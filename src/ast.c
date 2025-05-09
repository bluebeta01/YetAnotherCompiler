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
	case NODE_ASSIGN:
		return 50;
	case NODE_ADD:
		return 100;
	case NODE_SUBTRACT:
		return 100;
	case NODE_MULTIPLY:
		return 200;
	case NODE_VAR_DECL:
		return 300;
	case NODE_REF:
		return 300;
	case NODE_DEREF:
		return 300;
	case NODE_VAR:
		return 1000;
	case NODE_NUMBER:
		return 1000;

	}
	return 0;
}

//TODO: It's possible for the user to pass in bogus input with parentesis and cause memory leaks
Node *append_tree(Node *tree, Node *leaf)
{
	int leaf_precedence = node_precedence(leaf);

	Node *current_node = tree;
	while (node_precedence(current_node) < leaf_precedence && current_node->right && !current_node->paren)
		current_node = current_node->right;

	if ((node_precedence(current_node) < leaf_precedence && !current_node->paren) || leaf->paren) 
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
} AstBlockResult;

AstBlockResult ast_block(Vector *tokens, int index)
{
	ParseExpressionResult result = parse_expression(tokens, index);
}

bool ast_tokens(Vector *tokens)
{
	FunctionParseResult func_result = parse_function(tokens, 0);
	if (!func_result.success) return false;
	int index = func_result.next_index;
	if (index >= tokens->size) return false;

	Token *token = vec_at(Token*, tokens, index);
	if (token->type != TOKEN_OPEN_BRACE) return false;

	index++;
	if (index >= tokens->size) return false;

	AstBlockResult block_result = ast_block(tokens, index);
	return block_result.success;
}