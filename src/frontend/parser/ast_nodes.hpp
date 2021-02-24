#pragma once

#ifndef WOTPP_AST_NODES
#define WOTPP_AST_NODES

#include <utility>
#include <variant>
#include <string>
#include <vector>

#include <frontend/token.hpp>
#include <frontend/position.hpp>
#include <frontend/ast.hpp>


// AST nodes.
namespace wpp {
	// A function call.
	struct FnInvoke {
		std::string identifier;
		std::vector<wpp::node_t> arguments;
		wpp::Position pos;

		FnInvoke(
			const std::string& identifier_,
			const std::vector<wpp::node_t>& arguments_,
			const wpp::Position& pos_
		):
			identifier(identifier_),
			arguments(arguments_),
			pos(pos_) {}

		FnInvoke(const wpp::Position& pos_): pos(pos_) {}

		FnInvoke() {}
	};

	struct Intrinsic {
		wpp::token_type_t type;
		std::string identifier;
		std::vector<wpp::node_t> arguments;
		wpp::Position pos;

		Intrinsic(
			const wpp::token_type_t type_,
			const std::string& identifier_,
			const std::vector<wpp::node_t>& arguments_,
			const wpp::Position& pos_
		):
			type(type_),
			identifier(identifier_),
			arguments(arguments_),
			pos(pos_) {}

		Intrinsic(const wpp::Position& pos_): pos(pos_) {}

		Intrinsic() {}
	};

	// Function definition.
	struct Fn {
		std::string identifier;
		std::vector<std::string> parameters;
		wpp::node_t body;
		wpp::Position pos;

		Fn(
			const std::string& identifier_,
			const std::vector<std::string>& parameters_,
			const wpp::node_t body_,
			const wpp::Position& pos_
		):
			identifier(identifier_),
			parameters(parameters_),
			body(body_),
			pos(pos_) {}

		Fn(const wpp::Position& pos_): pos(pos_) {}

		Fn() {}
	};

	struct Codeify {
		wpp::node_t expr;
		wpp::Position pos;

		Codeify(
			const wpp::node_t expr_,
			const wpp::Position& pos_
		):
			expr(expr_),
			pos(pos_) {}

		Codeify(const wpp::Position& pos_): pos(pos_) {}

		Codeify() {}
	};

	// Variable definition.
	struct Var {
		std::string identifier;
		wpp::node_t body;
		wpp::Position pos;

		Var(
			const std::string& identifier_,
			const wpp::node_t body_,
			const wpp::Position& pos_
		):
			identifier(identifier_),
			body(body_),
			pos(pos_) {}

		Var(const wpp::Position& pos_): pos(pos_) {}

		Var() {}
	};

	struct Drop {
		wpp::node_t func;
		wpp::Position pos;

		Drop(const wpp::node_t& func_, const wpp::Position& pos_):
			func(func_), pos(pos_) {}

		Drop(const wpp::Position& pos_): pos(pos_) {}

		Drop() {}
	};

	// String literal.
	struct String {
		std::string value;
		wpp::Position pos;

		String(const std::string& value_, const wpp::Position& pos_):
			value(value_), pos(pos_) {}

		String(const wpp::Position& pos_): pos(pos_) {}

		String() {}
	};

	// Concatenation operator.
	struct Concat {
		wpp::node_t lhs, rhs;
		wpp::Position pos;

		Concat(wpp::node_t lhs_, wpp::node_t rhs_, const wpp::Position& pos_):
			lhs(lhs_), rhs(rhs_), pos(pos_) {}

		Concat(const wpp::Position& pos_): pos(pos_) {}

		Concat() {}
	};

	// Block of zero or more statements and trailing expression.
	struct Block {
		std::vector<wpp::node_t> statements;
		wpp::node_t expr;
		wpp::Position pos;

		Block(
			const std::vector<wpp::node_t>& statements_,
			const wpp::node_t expr_,
			const wpp::Position& pos_
		):
			statements(statements_),
			expr(expr_),
			pos(pos_) {}

		Block(const wpp::Position& pos_): pos(pos_) {}

		Block() {}
	};

	// Namespace that embodies zero or more statements.
	struct Pre {
		std::vector<wpp::node_t> exprs;
		std::vector<wpp::node_t> statements;
		wpp::Position pos;

		Pre(
			const std::vector<wpp::node_t>& exprs_,
			const std::vector<wpp::node_t>& statements_,
			const wpp::Position& pos_
		):
			exprs(exprs_),
			statements(statements_),
			pos(pos_) {}

		Pre(const wpp::Position& pos_): pos(pos_) {}

		Pre() {}
	};

	// Map strings to new strings.
	struct Map {
		wpp::node_t expr;
		std::vector<std::pair<wpp::node_t, wpp::node_t>> cases;
		wpp::node_t default_case;
		wpp::Position pos;

		Map(
			const wpp::node_t expr_,
			const std::vector<std::pair<wpp::node_t, wpp::node_t>>& cases_,
			const wpp::node_t default_case_,
			const wpp::Position& pos_
		):
			expr(expr_),
			cases(cases_),
			default_case(default_case_),
			pos(pos_) {}

		Map(const wpp::Position& pos_): pos(pos_) {}

		Map() {}
	};

	// The root node of a wot++ program.
	struct Document {
		std::vector<wpp::node_t> stmts;
		wpp::Position pos;

		Document(
			const std::vector<wpp::node_t>& stmts_,
			const wpp::Position& pos_
		):
			stmts(stmts_),
			pos(pos_) {}

		Document(const wpp::Position& pos_): pos(pos_) {}

		Document() {}
	};

	// An alias for our AST.
	using AST = wpp::HeterogenousVector<
		FnInvoke,
		Intrinsic,
		Fn,
		Var,
		Codeify,
		Map,
		String,
		Concat,
		Block,
		Pre,
		Document,
		Drop
	>;
}

#endif
