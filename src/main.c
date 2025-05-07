#include "list.h"
#include "tokenize.h"
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
			printf("INT: %ld\n", token->int_literal);
		}
	}

	ast_tokens(&tokens);
}