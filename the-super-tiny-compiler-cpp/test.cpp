#include "the-super-tiny-compiler.h"
#include <assert.h>


int main() {

	const auto input = "(add 2 (subtract 4 2))";
	const auto output = "add(2, subtract(4, 2));";

	Tokens tokens = {
		TokenPtr(new Token{"paren", "("}),
		TokenPtr(new Token{"name", "add"}),
		TokenPtr(new Token{"number", "2"}),
		TokenPtr(new Token{"paren", "("}),
		TokenPtr(new Token{"name", "subtract"}),
		TokenPtr(new Token{"number", "4"}),
		TokenPtr(new Token{"number", "2"}),
		TokenPtr(new Token{"paren", ")"}),
		TokenPtr(new Token{"paren", ")"}),
	};

	AstNodePtr ast = AstNodePtr(new ProgramNode{
		{
			AstNodePtr(new CallExpressionNode("add", {}, {
				AstNodePtr(new NumberLiteralNode("2")),
				AstNodePtr(new CallExpressionNode("subtract", {}, {
					AstNodePtr(new NumberLiteralNode("4")),
					AstNodePtr(new NumberLiteralNode("2"))
				}))
			}))
		},
	});

	AstNodePtr newAst = AstNodePtr(new ProgramNode{
		{
			AstNodePtr(new ExpressionStatementNode(
				std::shared_ptr<CallExpressionNode>(new CallExpressionNode(
					"",
					std::shared_ptr<IdentifierNode>(new IdentifierNode("add")),
					{
						AstNodePtr(new NumberLiteralNode("2")),
						AstNodePtr(new CallExpressionNode(
							"",
							std::shared_ptr<IdentifierNode>(new IdentifierNode("subtract")),
							{
								AstNodePtr(new NumberLiteralNode("4")),
								AstNodePtr(new NumberLiteralNode("2"))
							}
						))
					}
				))
			))
		},
	});

	assert(tokenizer(input) == tokens);
	assert(parser(tokens) == ast);
	assert(transformer(ast) == newAst);
	assert(code_generator(newAst) == output);
	assert(compiler(input) == output);
}
