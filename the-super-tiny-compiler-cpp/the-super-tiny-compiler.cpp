#include "the-super-tiny-compiler.h"

#include <ctype.h>
#include <exception>


Tokens tokenizer(const std::string& input) {
	size_t current = 0;
	Tokens tokens;

	while (current < input.size()) {
		char character = input[current];

		if (character == '(') {
			tokens.push_back(TokenPtr(new Token{ "paren", "(" }));
			current++;
			continue;
		}

		if (character == ')') {
			tokens.push_back(TokenPtr(new Token{ "paren", ")" }));
			current++;
			continue;
		}

		if (isspace(character)) {
			current++;
			continue;
		}

		if (isdigit(character)) {
			std::string value;
			while (current < input.size() && isdigit(input[current])) {
				value.push_back(input[current++]);
			}
			tokens.push_back(TokenPtr(new Token{ "number", value }));
			continue;
		}

		if (character == '"') {
			std::string value;
			while (++current < input.size() && input[current] != '"') {
				value.push_back(input[current]);
			}
			if (current >= input.size() || input[current] != '"') {
				throw new std::exception("parse string failed");
			}
			current++;
			tokens.push_back(TokenPtr(new Token{ "string", value }));
			continue;
		}

		if (isalpha(character)) {
			std::string value;
			while (current < input.size() && isalpha(input[current])) {
				value.push_back(input[current++]);
			}
			tokens.push_back(TokenPtr(new Token{ "name", value }));
			continue;
		}

		char msg[] = "I dont know what this character is:  ";
		msg[sizeof(msg) - 2] = character;
		throw new std::exception(msg);
	}

	return tokens;
}

AstNodePtr walk(size_t& current, const Tokens& tokens) {
	if (current >= tokens.size()) {
		throw new std::exception("out of order");
	}

	auto token = tokens[current];

	if (token->type == "number") {
		current++;
		return AstNodePtr(new NumberLiteralNode(token->value));
	}

	if (token->type == "string") {
		current++;
		return AstNodePtr(new StringLiteralNode(token->value));
	}

	if (token->type == "paren" && token->value == "(") {
		// skip (
		if (++current >= tokens.size()) {
			throw new std::exception("parentheses not match");
		}
		token = tokens[current];

		auto node = new CallExpressionNode(token->value, {});

		// skip name
		if (++current >= tokens.size()) {
			throw new std::exception("parentheses not match");
		}
		token = tokens[current];

		while (token->type != "paren" || token->value != ")") {
			node->params.push_back(walk(current, tokens));
			token = tokens[current];
		}

		current++;
		return AstNodePtr(node);
	}

	throw new std::exception(token->type.c_str());
}

AstNodePtr parser(const Tokens& tokens) {
	auto ast = new ProgramNode();
	size_t current = 0;

	while (current < tokens.size()) {
		ast->body.push_back(walk(current, tokens));
	}

	return AstNodePtr(ast);
}


typedef void(*visitor_method)(const AstNodePtr& node, AstNodePtr& parent);

struct VisitorMethod {
	visitor_method enter{};
	visitor_method exit{};
};

struct Visitor {
	std::map<std::string, VisitorMethod> methods;
};

VisitorMethod get_method(const Visitor& visitor, const std::string& method_name) {
	auto iter = visitor.methods.find(method_name);
	if (iter != visitor.methods.end())
		return iter->second;
	return {};
}

void NumberLiteralEnter(const AstNodePtr& node, AstNodePtr& parent) {
	auto newNode = AstNodePtr(new NumberLiteralNode(std::dynamic_pointer_cast<NumberLiteralNode>(node)->value));
	if (parent->type == "CallExpression") {
		std::dynamic_pointer_cast<CallExpressionNode>(parent)->params.push_back(newNode);
	}
}

void StringLiteralEnter(const AstNodePtr& node, AstNodePtr& parent) {
	auto newNode = AstNodePtr(new StringLiteralNode(std::dynamic_pointer_cast<StringLiteralNode>(node)->value));
	if (parent->type == "CallExpression") {
		std::dynamic_pointer_cast<CallExpressionNode>(parent)->params.push_back(newNode);
	}
}

void CallExpressionEnter(const AstNodePtr& node, AstNodePtr& parent) {
	auto newExpression = new CallExpressionNode("", {});
	newExpression->callee = std::shared_ptr<IdentifierNode>(new IdentifierNode(std::dynamic_pointer_cast<CallExpressionNode>(node)->name));
	AstNode* newNodeRaw = newExpression;
	AstNodePtr newNode, newParent;
	if (parent->type != "CallExpression") {
		// wrap this `CallExpression` with an `ExpressionStatement`
		auto statement = new ExpressionStatementNode();
		newParent = statement->expression = std::shared_ptr<CallExpressionNode>(newExpression);
		newNodeRaw = statement;
		newNode = AstNodePtr(newNodeRaw);
	} else {
		newParent = newNode = AstNodePtr(newNodeRaw);
	}

	if (parent->type == "CallExpression") {
		std::dynamic_pointer_cast<CallExpressionNode>(parent)->params.push_back(newNode);
	} else if (parent->type == "Program") {
		std::dynamic_pointer_cast<ProgramNode>(parent)->body.push_back(newNode);
	}
	parent = newParent;
}

void traverse_node(const AstNodePtr& node, AstNodePtr& parent, const Visitor& visitor);

void traverse_array(const AstNodes& nodes, AstNodePtr& parent, const Visitor& visitor) {
	for (const auto& child : nodes) {
		traverse_node(child, parent, visitor);
	}
}

void traverse_node(const AstNodePtr& node, AstNodePtr& parent, const Visitor& visitor) {
	auto method = get_method(visitor, node->type);
	if (method.enter) {
		method.enter(node, parent);
	}

	if (node->type == "Program") {
		auto program = std::dynamic_pointer_cast<ProgramNode>(node);
		traverse_array(program->body, parent, visitor);
	} else if (node->type == "CallExpression") {
		auto expression = std::dynamic_pointer_cast<CallExpressionNode>(node);
		traverse_array(expression->params, parent, visitor);
	} else if (node->type == "NumberLiteral" || node->type == "StringLiteral") {

	} else {
		throw new std::exception(node->type.c_str());
	}

	if (method.exit) {
		method.exit(node, parent);
	}
}

void traverse(const AstNodePtr& ast, AstNodePtr& parent, const Visitor& visitor) {
	AstNodePtr ref = parent;
	traverse_node(ast, ref, visitor);
}

AstNodePtr transformer(const AstNodePtr& ast) {
	auto newAst = AstNodePtr(new ProgramNode);
	Visitor visitor;
	visitor.methods["NumberLiteral"] = VisitorMethod{ NumberLiteralEnter, nullptr };
	visitor.methods["StringLiteral"] = VisitorMethod{ StringLiteralEnter, nullptr };
	visitor.methods["CallExpression"] = VisitorMethod{ CallExpressionEnter, nullptr };

	traverse(ast, newAst, visitor);

	return newAst;
}


std::string code_generator(const AstNodePtr& node) {
	if (node->type == "Program") {
		auto program = std::dynamic_pointer_cast<ProgramNode>(node);
		std::string code;
		auto begin = program->body.cbegin();
		auto end = program->body.cend();
		if (begin != end) {
			code += code_generator(*begin++);
		}
		while (begin != end) {
			code += "\n";
			code += code_generator(*begin++);
		}
		return code;
	} else if (node->type == "ExpressionStatement") {
		return code_generator(std::dynamic_pointer_cast<ExpressionStatementNode>(node)->expression) + ";";
	} else if (node->type == "CallExpression") {
		auto expression = std::dynamic_pointer_cast<CallExpressionNode>(node);
		std::string code = code_generator(expression->callee);
		code += "(";

		auto begin = expression->params.cbegin();
		auto end = expression->params.cend();
		if (begin != end) {
			code += code_generator(*begin++);
		}
		while (begin != end) {
			code += ", ";
			code += code_generator(*begin++);
		}

		code += ")";
		return code;
	} else if (node->type == "Identifier") {
		return std::dynamic_pointer_cast<IdentifierNode>(node)->name;
	} else if (node->type == "NumberLiteral") {
		return std::dynamic_pointer_cast<NumberLiteralNode>(node)->value;
	} else if (node->type == "StringLiteral") {
		return '"' + std::dynamic_pointer_cast<StringLiteralNode>(node)->value + '"';
	} else {
		throw new std::exception(node->type.c_str());
	}
}

std::string compiler(const std::string& input) {
	auto tokens = tokenizer(input);
	auto ast = parser(tokens);
	auto newAst = transformer(ast);
	auto output = code_generator(newAst);
	return output;
}

bool operator==(const TokenPtr& lhs, const TokenPtr& rhs) {
	if (!lhs && !rhs) {
		return true;
	} else if (lhs && rhs) {
		return lhs->type == rhs->type && lhs->value == rhs->value;
	} else {
		return false;
	}
}

bool operator==(const AstNodePtr& lhs, const AstNodePtr& rhs) {
	if (!lhs && !rhs) {
		return true;
	} else if (lhs && rhs) {
		if (lhs->type != rhs->type)
			return false;
		if (lhs->type == "NumberLiteral") {
			return std::dynamic_pointer_cast<NumberLiteralNode>(lhs)->value == std::dynamic_pointer_cast<NumberLiteralNode>(rhs)->value;
		} else if (lhs->type == "StringLiteral") {
			return std::dynamic_pointer_cast<StringLiteralNode>(lhs)->value == std::dynamic_pointer_cast<StringLiteralNode>(rhs)->value;
		} else if (lhs->type == "Identifier") {
			return std::dynamic_pointer_cast<IdentifierNode>(lhs)->name == std::dynamic_pointer_cast<IdentifierNode>(rhs)->name;
		} else if (lhs->type == "CallExpression") {
			auto lex = std::dynamic_pointer_cast<CallExpressionNode>(lhs);
			auto rex = std::dynamic_pointer_cast<CallExpressionNode>(rhs);
			if (lex->name != rex->name)
				return false;
			if (!(AstNodePtr(lex->callee) == AstNodePtr(rex->callee)))
				return false;
			if (lex->params != rex->params)
				return false;

			return true;
		} else if (lhs->type == "ExpressionStatement") {
			return AstNodePtr(std::dynamic_pointer_cast<ExpressionStatementNode>(lhs)->expression) ==
				AstNodePtr(std::dynamic_pointer_cast<ExpressionStatementNode>(rhs)->expression);
		} else if (lhs->type == "Program") {
			return std::dynamic_pointer_cast<ProgramNode>(lhs)->body == std::dynamic_pointer_cast<ProgramNode>(rhs)->body;
		} else {
			return false;
		}
	} else {
		return false;
	}
}
//
//bool operator==(const AstNodes& lhs, const AstNodes& rhs) {
//
//}
