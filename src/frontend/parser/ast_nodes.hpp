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
	struct Intrinsic {
		std::vector<wpp::node_t> arguments;
		wpp::View identifier;
		wpp::token_type_t type;

		Intrinsic(
			const std::vector<wpp::node_t>& arguments_,
			const wpp::View& identifier_,
			const wpp::token_type_t type_
		):
			arguments(arguments_),
			identifier(identifier_),
			type(type_) {}

		Intrinsic() {}
	};

	// A function call.
	struct FnInvoke {
		std::vector<wpp::node_t> arguments;
		wpp::View identifier;

		FnInvoke(
			const std::vector<wpp::node_t>& arguments_,
			const wpp::View& identifier_
		):
			arguments(arguments_),
			identifier(identifier_) {}

		FnInvoke() {}
	};

	// Function definition.
	struct Fn {
		std::vector<wpp::View> parameters;
		wpp::View identifier;
		wpp::node_t body;

		Fn(
			const std::vector<wpp::View>& parameters_,
			const wpp::View& identifier_,
			const wpp::node_t body_
		):
			parameters(parameters_),
			identifier(identifier_),
			body(body_) {}

		Fn() {}
	};

	// A variable reference.
	struct VarRef {
		wpp::View identifier;

		VarRef(const wpp::View& identifier_): identifier(identifier_) {}
		VarRef() {}
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

	struct Codeify {
		wpp::node_t expr;

		Codeify(const wpp::node_t expr_): expr(expr_) {}
		Codeify() {}
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

	// Map strings to new strings.
	struct Map {
		std::vector<std::pair<wpp::node_t, wpp::node_t>> cases;
		wpp::node_t expr;
		wpp::node_t default_case;

		Map(
			const std::vector<std::pair<wpp::node_t, wpp::node_t>>& cases_,
			const wpp::node_t expr_,
			const wpp::node_t default_case_
		):
			cases(cases_),
			expr(expr_),
			default_case(default_case_) {}

		Map() {}
	};

	struct Use {
		wpp::node_t path;

		Use(const wpp::node_t path_): path(path_) {}
		Use() {}
	};

	// The root node of a wot++ program.
	struct Document {
		std::vector<wpp::node_t> statements;

		Document(const std::vector<wpp::node_t>& statements_): statements(statements_) {}
		Document() {}
	};

	// An alias for our AST.
	using AST = wpp::HeterogenousVector<
		Use,
		FnInvoke,
		Fn,
		VarRef,
		Var,
		Intrinsic,
		Codeify,
		Map,
		String,
		Concat,
		Block,
		Document,
		Drop
	>;
}

#endif
