#include "parser.h"
#include <stdlib.h>
#include <stdio.h>

#define ERRMSG_SIZE 100
char errmsg[ERRMSG_SIZE];
TokenType expected_token;
int parse_error = 0;
Token *error_token = NULL;

#define VerifyTokenType(token, token_type) \
	if ((token)->type != token_type)       \
	{                                      \
		expected_token = token_type;       \
		return PR_UNEXPECTED_TOKEN;        \
	}
#define FetchNextToken(vector, token, index) \
	index++;                                 \
	if (index >= (vector)->size)             \
		return PR_EOF;                       \
	token = vec_at(Token *, vector, index);

ParseResult parse_declaration(Vector *tokens, int index, int *next_index, struct Declaration *decl);
ParseResult parse_type_descriptor(Vector *tokens, int index, int *next_index, struct TypeDescriptor *td);

void parser_error_msg(const char *msg, Token *token, ParseResult result)
{
	parse_error = result;
	snprintf(errmsg, ERRMSG_SIZE, "(%i, %i): %s", token->line, token->column, msg);
}

void parser_error(Token *token, ParseResult result)
{
	parse_error = result;
	error_token = token;
}

void print_parser_error()
{
	switch (parse_error)
	{
	case PR_OK:
		return;
	case PR_EOF:
		puts("Unexpected end of file");
		return;
	default:
		printf("Failed to compile.\n%s\n", errmsg);
		break;
	}
}

// Prints an ast tree for debugging
void pretty_print_tree(struct AstNode *tree)
{
	if (tree->left)
		pretty_print_tree(tree->left);
	if (tree->right)
		pretty_print_tree(tree->right);

	switch (tree->type)
	{
	case AST_NODE_SEQ:
		puts("SEQ");
		break;
	case AST_NODE_VALUE:
		puts(tree->token->name);
		break;
	case AST_NODE_VAR_DECL:
		puts("DECL");
		break;
	case AST_NODE_MUL:
		puts("MUL");
		break;
	case AST_NODE_ADD:
		puts("ADD");
		break;
	case AST_NODE_SUB:
		puts("SUB");
		break;
	case AST_NODE_ASSIGN:
		puts("ASSIGN");
		break;

	default:
		puts("UNKNOWN NODE TYPE");
	}
}

// Prints a function descriptor and it's body for debugging
void pretty_print_function(struct FuncDescriptor *func)
{
	printf("fn %s:\n", func->declaration.token->name);
	if (func->code_block.tree == NULL)
	{
		puts("Function has no body");
		return;
	}

	pretty_print_tree(func->code_block.tree);
}

// Parses a function type descriptor, allocating it on the heap and storing in func_td
ParseResult parse_func_type_descriptor(Vector *tokens, int index, int *next_index, struct FuncTypeDescriptor **func_td)
{
	Token *token = vec_at(Token *, tokens, index);
	VerifyTokenType(token, TOKEN_OPEN_PAREN);
	FetchNextToken(tokens, token, index);
	Vector params = vec_new(struct Declaration, 1);

	while (token->type != TOKEN_CLOSE_PAREN)
	{
		struct Declaration param_decl = (struct Declaration){0};
		ParseResult result = parse_declaration(tokens, index, next_index, &param_decl);
		if (result)
		{
			goto err_cleanup;
		}
		vec_push(struct Declaration, &params, &param_decl);
		index = *next_index;
		if (index >= tokens->size)
		{
			parse_error = PR_EOF;
			goto err_cleanup;
		}
		token = vec_at(Token *, tokens, index);
		if (token->type == TOKEN_COMMA)
		{
			index++;
			if (index >= tokens->size)
			{
				parse_error = PR_EOF;
				goto err_cleanup;
			}
			token = vec_at(Token *, tokens, index);
		}
	}

	index++;
	if (index >= tokens->size)
	{
		parse_error = PR_EOF;
		goto err_cleanup;
	}
	token = vec_at(Token *, tokens, index);
	if (token->type != TOKEN_ARROW)
	{
		parser_error_msg("Expected '->' token in function declaration", token, PR_UNEXPECTED_TOKEN);
		goto err_cleanup;
	}
	index++;
	if (index >= tokens->size)
	{
		parse_error = PR_EOF;
		goto err_cleanup;
	}

	struct TypeDescriptor return_td = (struct TypeDescriptor){0};
	ParseResult result = parse_type_descriptor(tokens, index, next_index, &return_td);
	if (result)
	{
		goto err_cleanup;
	}

	*func_td = malloc(sizeof(struct FuncTypeDescriptor));
	**func_td = (struct FuncTypeDescriptor){
		.return_type = return_td,
		.parameter_declarations = params};
	return PR_OK;

err_cleanup:
	vec_free(&params);
	return parse_error;
}

// Parses a type descriptor at stores in td
ParseResult parse_type_descriptor(Vector *tokens, int index, int *next_index, struct TypeDescriptor *td)
{
	Token *token = vec_at(Token *, tokens, index);

	int ptr_count = 0;
	while (true)
	{
		if (token->type == TOKEN_STAR)
			ptr_count++;
		else
			break;
		FetchNextToken(tokens, token, index);
	}

	// Check if this is a function type descriptor
	if (token->type == TOKEN_OPEN_PAREN)
	{
		Token *name = token;
		struct FuncTypeDescriptor *func_td = NULL;
		ParseResult r = parse_func_type_descriptor(tokens, index, next_index, &func_td);
		if (r)
			return r;
		*td = (struct TypeDescriptor){
			.base = BASE_TYPE_FUNC,
			.ptr_count = ptr_count,
			.name_token = name,
			.func_type_descriptor = func_td};
		return PR_OK;
	}

	enum BaseType base = BASE_TYPE_UNKNOWN;
	switch (token->type)
	{
	case TOKEN_U8:
		base = BASE_TYPE_U8;
		break;
	case TOKEN_I8:
		base = BASE_TYPE_I8;
		break;
	case TOKEN_U16:
		base = BASE_TYPE_U16;
		break;
	case TOKEN_I16:
		base = BASE_TYPE_I16;
		break;
	case TOKEN_VOID:
		base = BASE_TYPE_VOID;
		break;
	}
	Token *name_token = token;

	*td = (struct TypeDescriptor){
		.base = base,
		.ptr_count = ptr_count,
		.name_token = name_token};
	*next_index = index;
	return PR_OK;
}

// Parses a declaration (an identifier followed by a :type) and stores in decl
ParseResult parse_declaration(Vector *tokens, int index, int *next_index, struct Declaration *decl)
{
	Token *token = vec_at(Token *, tokens, index);
	VerifyTokenType(token, TOKEN_IDENTIFIER);
	Token *name = token;
	FetchNextToken(tokens, token, index);
	VerifyTokenType(token, TOKEN_COLON);
	FetchNextToken(tokens, token, index);

	struct TypeDescriptor td = (struct TypeDescriptor){0};
	ParseResult result = parse_type_descriptor(tokens, index, next_index, &td);
	if (result)
		return result;
	index = *next_index;
	if (index >= tokens->size)
		return PR_EOF;

	*decl = (struct Declaration){
		.type = td,
		.token = name};
	*next_index = index + 1;
	return PR_OK;
}

// Parses a typedef declaration and adds it to the parser context
ParseResult parse_typedef(struct ParserContext *ctx, Vector *tokens, int index, int *next_index)
{
	Token *token = vec_at(Token *, tokens, index);
	VerifyTokenType(token, TOKEN_TYPEDEF);
	FetchNextToken(tokens, token, index);
	struct Declaration typedef_decl = (struct Declaration){0};
	ParseResult result = parse_declaration(tokens, index, next_index, &typedef_decl);
	if (result)
		return result;
	index = *next_index;
	if (index >= tokens->size)
		return PR_EOF;
	token = vec_at(Token *, tokens, index);
	VerifyTokenType(token, TOKEN_SEMICOLON);
	vec_push(struct Declaration, &ctx->defined_types, &typedef_decl);
	*next_index = index + 1;
	return PR_OK;
}

// Generates the next ast node from tokens, storing it in node. Prev_node should be the previously generated node or NULL if there is none
ParseResult parse_node(Vector *tokens, int index, int *next_index, struct AstNode *prev_node, struct AstNode **out_node)
{
	Token *token = vec_at(Token *, tokens, index);
	if (token->type == TOKEN_LET)
	{
		FetchNextToken(tokens, token, index);
		struct AstVarDecl *var_decl = malloc(sizeof(struct AstVarDecl));
		*var_decl = (struct AstVarDecl){0};
		ParseResult result = parse_declaration(tokens, index, next_index, &var_decl->declaration);
		if (result)
		{
			free(var_decl);
			return result;
		}
		struct AstNode *node = malloc(sizeof(struct AstNode));
		*node = (struct AstNode){
			.type = AST_NODE_VAR_DECL,
			.token = var_decl->declaration.token,
			.var_decl = var_decl};

		*out_node = node;
		return PR_OK;
	}

	if (token->type == TOKEN_EQUAL)
	{
		struct AstNode *node = malloc(sizeof(struct AstNode));
		*node = (struct AstNode){
			.type = AST_NODE_ASSIGN,
			.token = token};

		*next_index = index + 1;
		*out_node = node;
		return PR_OK;
	}

	if (token->type == TOKEN_INT || token->type == TOKEN_IDENTIFIER)
	{
		struct AstNode *node = malloc(sizeof(struct AstNode));
		*node = (struct AstNode){
			.type = AST_NODE_VALUE,
			.token = token};

		*next_index = index + 1;
		*out_node = node;
		return PR_OK;
	}

	if (token->type == TOKEN_PLUS)
	{
		struct AstNode *node = malloc(sizeof(struct AstNode));
		*node = (struct AstNode){
			.type = AST_NODE_ADD,
			.token = token};

		*next_index = index + 1;
		*out_node = node;
		return PR_OK;
	}

	if (token->type == TOKEN_STAR)
	{
		struct AstNode *node = malloc(sizeof(struct AstNode));
		*node = (struct AstNode){
			.type = AST_NODE_MUL,
			.token = token};

		*next_index = index + 1;
		*out_node = node;
		return PR_OK;
	}

	parser_error_msg("Unexpected token in expression", token, PR_UNEXPECTED_TOKEN);
	return PR_UNEXPECTED_TOKEN;
}

// Get the precendence of a node type
int node_precedence(enum AstNodeType type)
{
	switch (type)
	{
	case AST_NODE_VALUE:
	case AST_NODE_VAR_DECL:
		return 0;

	case AST_NODE_MUL:
		return 1;

	case AST_NODE_ADD:
	case AST_NODE_SUB:
		return 2;

	case AST_NODE_ASSIGN:
		return 3;

	default:
		return 0;
	}
}

// Appends the leaf to the tree and returns the new tree head in new_head
bool append_node(struct AstNode *tree, struct AstNode *leaf, struct AstNode **new_head)
{
	int leaf_precedence = node_precedence(leaf->type);
	struct AstNode *target_node = tree;
	*new_head = tree;

	while (target_node->right != NULL && node_precedence(target_node->type) > leaf_precedence && !target_node->paren)
		target_node = target_node->right;

	int target_node_precedence = node_precedence(target_node->type);

	// If the target node and the leaf node have the paren flag set, the tree is invalid
	if (target_node->paren && leaf->paren)
	{
		return false;
	}

	// If the target node has the paren flag set, the leaf must be inserted at it's position
	if (target_node->paren)
	{
		leaf->left = target_node;
		leaf->parent = target_node->parent;
		if (target_node->parent == NULL)
		{
			*new_head = leaf;
			return true;
		}
		target_node->parent->right = leaf;
		return true;
	}

	// If the leaf has the paren flag set, it must be inserted below the target node
	if (leaf->paren)
	{
		if (target_node->right)
			return false;
		target_node->right = leaf;
		leaf->parent = target_node;
		return true;
	}

	// If true, the leaf needs to be inserted at the target's position or just below it
	if (target_node_precedence >= leaf_precedence)
	{
		// Insert the leaf as the right child of the target
		if (target_node->right == NULL)
		{
			target_node->right = leaf;
			leaf->parent = target_node;
			return true;
		}

		// Insert the leaf in the target node's position
		leaf->left = target_node;
		leaf->parent = target_node->parent;
		if (target_node->parent == NULL)
		{
			*new_head = leaf;
			return true;
		}
		target_node->parent->right = leaf;
		return true;
	}

	// The leaf needs to be inserted at the target nodes position
	leaf->left = target_node;
	leaf->parent = target_node->parent;
	if (target_node->parent == NULL)
	{
		*new_head = leaf;
		return true;
	}
	target_node->parent->right = leaf;
	return true;
}

// Parses a statement into an AST node tree, leaving a pointer to the head node in tree
ParseResult parse_statement(Vector *tokens, int index, int *next_index, struct AstNode **out_tree)
{
	Token *token = vec_at(Token *, tokens, index);
	struct AstNode *tree = NULL;
	while (true)
	{
		token = vec_at(Token *, tokens, index);

		if (token->type == TOKEN_CLOSE_PAREN)
		{
			*next_index = index;
			break;
		}

		if (token->type == TOKEN_SEMICOLON)
		{
			*next_index = index + 1;
			break;
		}

		struct AstNode *node = NULL;

		if (token->type == TOKEN_OPEN_PAREN)
		{
			index++;
			if (index >= tokens->size)
			{
				parse_error = PR_EOF;
				goto err_cleanup;
			}
			ParseResult result = parse_statement(tokens, index, next_index, &node);
			if (result)
			{
				goto err_cleanup;
			}
			if (node == NULL)
			{
				goto err_cleanup;
			}
			index = *next_index;
			if (index >= tokens->size)
			{
				parse_error = PR_EOF;
				goto err_cleanup;
			}
			token = vec_at(Token *, tokens, index);
			if (token->type != TOKEN_CLOSE_PAREN)
			{
				parser_error_msg("Expected closing paren", token, PR_UNEXPECTED_TOKEN);
				goto err_cleanup;
			}
			index++;
			if (index >= tokens->size)
			{
				parse_error = PR_EOF;
				goto err_cleanup;
			}
			node->paren = true;
		}
		else
		{
			ParseResult result = parse_node(tokens, index, next_index, NULL, &node);
			if (result)
			{
				goto err_cleanup;
			}
			index = *next_index;
			if (index >= tokens->size)
			{
				parse_error = PR_EOF;
				goto err_cleanup;
			}
		}

		if (!tree)
		{
			tree = node;
		}
		else
		{
			struct AstNode *new_head;
			bool result = append_node(tree, node, &new_head);
			if (result)
			{
				tree = new_head;
			}
			else
			{
				parser_error_msg("Appending node to the AST failed", token, PR_INTERNAL_ERROR);
				goto err_cleanup;
			}
		}
	}

	*out_tree = tree;
	return PR_OK;

err_cleanup:
	// TODO: cleanup
	return parse_error;
}

// Parses a code block and store it in block
ParseResult parse_block(struct ParserContext *ctx, Vector *tokens, int index, int *next_index, struct CodeBlock *block)
{
	Token *token = vec_at(Token *, tokens, index);
	VerifyTokenType(token, TOKEN_OPEN_BRACE);
	FetchNextToken(tokens, token, index);

	struct AstNode *tree = NULL;
	while (token->type != TOKEN_CLOSE_BRACE)
	{
		struct AstNode *statement = NULL;
		ParseResult result = parse_statement(tokens, index, next_index, &statement);
		if (result)
		{
			goto err_cleanup;
		}
		if (!statement)
		{
			parser_error_msg("Invalid expression", token, PR_SYNTAX);
			goto err_cleanup;
		}

		if (tree == NULL)
		{
			tree = statement;
		}
		else
		{
			struct AstNode *seq = malloc(sizeof(struct AstNode));
			*seq = (struct AstNode){
				.type = AST_NODE_SEQ,
				.left = tree,
				.right = statement};
			tree->parent = seq;
			statement->parent = seq;
			tree = seq;
		}

		index = *next_index;
		if (index >= tokens->size)
		{
			parse_error = PR_EOF;
			goto err_cleanup;
		}
		token = vec_at(Token *, tokens, index);
	}

	*block = (struct CodeBlock){
		.tree = tree};
	*next_index = index + 1;
	return PR_OK;

err_cleanup:
	// TODO: cleanup
	return parse_error;
}

// Parses a function and store the descriptor in the parser context
ParseResult parse_function(struct ParserContext *ctx, Vector *tokens, int index, int *next_index)
{
	Token *token = vec_at(Token *, tokens, index);
	VerifyTokenType(token, TOKEN_FN);
	FetchNextToken(tokens, token, index);
	struct FuncDescriptor func_desc = (struct FuncDescriptor){0};
	ParseResult result = parse_declaration(tokens, index, next_index, &func_desc.declaration);
	if (result)
		return result;
	index = *next_index;
	if (index >= tokens->size)
	{
		parse_error = PR_EOF;
		goto err_cleanup;
	}

	result = parse_block(ctx, tokens, index, next_index, &func_desc.code_block);
	if (result)
		goto err_cleanup;

	vec_push(struct FuncDescriptor, &ctx->functions, &func_desc);
	return PR_OK;

err_cleanup:
	// TODO: Free the assocated resources appropiratly
	return parse_error;
}

ParseResult parse_file(struct ParserContext *ctx, Vector *tokens)
{
	int index = 0;
	int next_index = 0;
	ParseResult result = false;

	while (index < tokens->size - 1)
	{
		Token *token = vec_at(Token *, tokens, index);

		if (token->type == TOKEN_TYPEDEF)
		{
			result = parse_typedef(ctx, tokens, index, &next_index);
			if (result)
				return result;
			index = next_index;
			continue;
		}

		if (token->type == TOKEN_FN)
		{
			result = parse_function(ctx, tokens, index, &next_index);
			if (result)
				return result;
			index = next_index;
			continue;
		}

		break;
	}

	for (int i = 0; i < ctx->functions.size; i++)
	{
		struct FuncDescriptor *func = &vec_at(struct FuncDescriptor, &ctx->functions, i);
		pretty_print_function(func);
	}

	if (index != tokens->size)
	{
		return PR_EOF;
	}

	return PR_OK;
}

struct ParserContext parser_init()
{
	struct ParserContext ctx = (struct ParserContext){
		.defined_types = vec_new(struct Declaration, 10),
		.functions = vec_new(struct FuncDescriptor, 10),
	};
	return ctx;
}

void parser_deinit(struct ParserContext *ctx)
{
	vec_free(&ctx->defined_types);
	vec_free(&ctx->functions);
}