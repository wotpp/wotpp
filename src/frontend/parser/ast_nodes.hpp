#pragma once

#ifndef WOTPP_AST_NODES
#define WOTPP_AST_NODES

#include <variant>
#include <string>
#include <vector>

#include <frontend/token.hpp>
#include <frontend/view.hpp>
#include <frontend/ast.hpp>


// AST nodes.
namespace wpp {
	// A function call.
	struct FnInvoke {
		wpp::View identifier;
		std::vector<wpp::node_t> arguments;

		FnInvoke(
			const wpp::View& identifier_,
			const std::vector<wpp::node_t>& arguments_
		):
			identifier(identifier_),
			arguments(arguments_) {}

		FnInvoke() {}
	};

	struct Intrinsic {
		wpp::token_type_t type;
		wpp::View identifier;
		std::vector<wpp::node_t> arguments;

		Intrinsic(
			const wpp::token_type_t type_,
			const wpp::View& identifier_,
			const std::vector<wpp::node_t>& arguments_
		):
			type(type_),
			identifier(identifier_),
			arguments(arguments_) {}

		Intrinsic() {}
	};

	// Function definition.
	struct Fn {
		wpp::View identifier;
		std::vector<wpp::View> parameters;
		wpp::node_t body;

		Fn(
			const wpp::View& identifier_,
			const std::vector<wpp::View>& parameters_,
			const wpp::node_t body_
		):
			identifier(identifier_),
			parameters(parameters_),
			body(body_) {}

		Fn() {}
	};

	struct Codeify {
		wpp::node_t expr;

		Codeify(const wpp::node_t expr_): expr(expr_) {}
		Codeify() {}
	};

	// Variable definition.
	struct Var {
		wpp::View identifier;
		wpp::node_t body;

		Var(
			const wpp::View& identifier_,
			const wpp::node_t body_
		):
			identifier(identifier_),
			body(body_) {}

		Var() {}
	};

	struct Drop {
		wpp::node_t func;

		Drop(const wpp::node_t& func_): func(func_) {}
		Drop() {}
	};

	// String literal.
	struct String {
		std::string value;

		String(const std::string& value_): value(value_) {}
		String() {}
	};

	// Concatenation operator.
	struct Concat {
		wpp::node_t lhs, rhs;

		Concat(wpp::node_t lhs_, wpp::node_t rhs_): lhs(lhs_), rhs(rhs_) {}
		Concat() {}
	};

	// Block of zero or more statements and trailing expression.
	struct Block {
		std::vector<wpp::node_t> statements;
		wpp::node_t expr;

		Block(
			const std::vector<wpp::node_t>& statements_,
			const wpp::node_t expr_
		):
			statements(statements_),
			expr(expr_) {}

		Block() {}
	};

	// Namespace that embodies zero or more statements.
	struct Pre {
		std::vector<wpp::node_t> exprs;
		std::vector<wpp::node_t> statements;

		Pre(
			const std::vector<wpp::node_t>& exprs_,
			const std::vector<wpp::node_t>& statements_
		):
			exprs(exprs_),
			statements(statements_) {}

		Pre() {}
	};

	// Map strings to new strings.
	struct Map {
		wpp::node_t expr;
		std::vector<std::pair<wpp::node_t, wpp::node_t>> cases;
		wpp::node_t default_case;

		Map(
			const wpp::node_t expr_,
			const std::vector<std::pair<wpp::node_t, wpp::node_t>>& cases_,
			const wpp::node_t default_case_
		):
			expr(expr_),
			cases(cases_),
			default_case(default_case_) {}

		Map() {}
	};

	// The root node of a wot++ program.
	struct Document {
		std::vector<wpp::node_t> stmts;

		Document(const std::vector<wpp::node_t>& stmts_): stmts(stmts_) {}
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
