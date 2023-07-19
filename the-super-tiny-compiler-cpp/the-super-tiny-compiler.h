#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>

struct Token {
	std::string type{};
	std::string value{};
};

using TokenPtr = std::shared_ptr<Token>;
using Tokens = std::vector<TokenPtr>;

struct AstNode;
using AstNodePtr = std::shared_ptr<AstNode>;
using AstNodes = std::vector<AstNodePtr>;

struct AstNode {
	AstNode(const std::string& type) : type(type) {}
	virtual ~AstNode() {}

	std::string type{};
};

struct NumberLiteralNode : public AstNode {
	NumberLiteralNode(const std::string& value) : AstNode("NumberLiteral"), value(value) {}
	std::string value{};
};

struct StringLiteralNode : public AstNode {
	StringLiteralNode(const std::string& value) : AstNode("StringLiteral"), value(value) {}
	std::string value{};
};

struct IdentifierNode : public AstNode {
	IdentifierNode(const std::string& name) : AstNode("Identifier"), name(name) {}
	std::string name{};
};

struct CallExpressionNode : public AstNode {
	CallExpressionNode(const std::string& name = {}, const std::shared_ptr<IdentifierNode>& callee = {}, const AstNodes& params = {})
		: AstNode("CallExpression"), name(name), callee(callee), params(params) {}
	std::string name{};
	std::shared_ptr<IdentifierNode> callee{};
	AstNodes params{};
};

struct ExpressionStatementNode : public AstNode {
	ExpressionStatementNode(const std::shared_ptr<CallExpressionNode>& expression = {})
		: AstNode("ExpressionStatement"), expression(expression) {}

	std::shared_ptr<CallExpressionNode> expression{};
};

struct ProgramNode : public AstNode {
	ProgramNode(const AstNodes& body = {}) : AstNode("Program"), body(body) {}
	AstNodes body{};
};

Tokens tokenizer(const std::string& input);
AstNodePtr parser(const Tokens& tokens);
AstNodePtr transformer(const AstNodePtr& ast);
std::string code_generator(const AstNodePtr& node);
std::string compiler(const std::string& input);

bool operator==(const TokenPtr& lhs, const TokenPtr& rhs);
bool operator==(const AstNodePtr& lhs, const AstNodePtr& rhs);
//bool operator==(const AstNodes& lhs, const AstNodes& rhs);
