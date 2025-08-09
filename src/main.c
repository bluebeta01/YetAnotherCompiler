#include "list.h"
#include "tokenize.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

int main()
{
	Vector tokens = vec_new(Token*, 50);
	tokenize_file("/code/kc_test.txt", &tokens);

	for (int i = 0; i < tokens.size; i++)
	{
		Token *token = vec_at(Token*, &tokens, i);
		printf("%s\t\t\t%d\t%d\n", token->name, token->line, token->column);
		if (token->type == TOKEN_INT)
		{
			printf("INT: %lld\n", token->int_literal);
		}
	}

//	struct CompilerContext ctx = compiler_create_context();
//	bool r = compile_tokens(&ctx, tokens.data, 0, tokens.size);

	//ast_tokens(&tokens);

	struct ParserContext pctx = parser_init();
	ParseResult r = parse_file(&pctx, &tokens);
	if (r)
	{
		print_parser_error();
	}
	parser_deinit(&pctx);
}