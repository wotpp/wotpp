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
	struct IntrinsicFile {
		wpp::node_t expr{};

		IntrinsicFile(wpp::node_t expr_): expr(expr_) {}
		IntrinsicFile() {}
	};

	struct IntrinsicUse {
		wpp::node_t expr{};

		IntrinsicUse(wpp::node_t expr_): expr(expr_) {}
		IntrinsicUse() {}
	};

	struct IntrinsicRun {
		wpp::node_t expr{};

		IntrinsicRun(wpp::node_t expr_): expr(expr_) {}
		IntrinsicRun() {}
	};

	struct IntrinsicLog {
		wpp::node_t expr{};

		IntrinsicLog(wpp::node_t expr_): expr(expr_) {}
		IntrinsicLog() {}
	};

	struct IntrinsicError {
		wpp::node_t expr{};

		IntrinsicError(wpp::node_t expr_): expr(expr_) {}
		IntrinsicError() {}
	};

	struct IntrinsicPipe {
		wpp::node_t cmd{};
		wpp::node_t value{};

		IntrinsicPipe(wpp::node_t cmd_, wpp::node_t value_): cmd(cmd_), value(value_) {}
		IntrinsicPipe() {}
	};

	struct IntrinsicAssert {
		wpp::node_t lhs{};
		wpp::node_t rhs{};

		IntrinsicAssert(wpp::node_t lhs_, wpp::node_t rhs_): lhs(lhs_), rhs(rhs_) {}
		IntrinsicAssert() {}
	};

	// A function call.
	struct FnInvoke {
		std::vector<wpp::node_t> arguments{};
		wpp::View identifier{};

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
		std::vector<wpp::View> parameters{};
		wpp::View identifier{};
		wpp::node_t body{};

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
		wpp::View identifier{};

		VarRef(const wpp::View& identifier_): identifier(identifier_) {}
		VarRef() {}
	};

	// Variable definition.
	struct Var {
		wpp::View identifier{};
		wpp::node_t body{};

		Var(
			const wpp::View& identifier_,
			const wpp::node_t body_
		):
			identifier(identifier_),
			body(body_) {}

		Var() {}
	};

	struct Codeify {
		wpp::node_t expr{};

		Codeify(const wpp::node_t expr_): expr(expr_) {}
		Codeify() {}
	};


	struct Drop {
		wpp::View identifier{};
		size_t n_args{};
		bool is_variadic{};

		Drop(const wpp::View& identifier_, size_t n_args_, bool is_variadic_):
			identifier(identifier_), n_args(n_args_), is_variadic(is_variadic_) {}

		Drop() {}
	};


	// String literal.
	struct String {
		std::string value{};

		String(const std::string& value_): value(value_) {}
		String() {}
	};

	// Concatenation operator.
	struct Concat {
		wpp::node_t lhs{}, rhs{};

		Concat(wpp::node_t lhs_, wpp::node_t rhs_): lhs(lhs_), rhs(rhs_) {}
		Concat() {}
	};

	// Block of zero or more statements and trailing expression.
	struct Block {
		std::vector<wpp::node_t> statements{};
		wpp::node_t expr{};

		Block(
			const std::vector<wpp::node_t>& statements_,
			const wpp::node_t expr_
		):
			statements(statements_),
			expr(expr_) {}

		Block() {}
	};

	// Match strings to new strings.
	struct Match {
		std::vector<std::pair<wpp::node_t, wpp::node_t>> cases{};
		wpp::node_t expr{};
		wpp::node_t default_case{};

		Match(
			const std::vector<std::pair<wpp::node_t, wpp::node_t>>& cases_,
			const wpp::node_t expr_,
			const wpp::node_t default_case_
		):
			cases(cases_),
			expr(expr_),
			default_case(default_case_) {}

		Match() {}
	};

	struct Use {
		wpp::node_t path{};

		Use(const wpp::node_t path_): path(path_) {}
		Use() {}
	};

	// The root node of a wot++ program.
	struct Document {
		std::vector<wpp::node_t> statements{};

		Document(const std::vector<wpp::node_t>& statements_): statements(statements_) {}
		Document() {}
	};

	struct Pop {
		std::vector<wpp::node_t> arguments{};
		wpp::View identifier{};
		size_t n_popped_args{};

		Pop(
			const std::vector<wpp::node_t>& arguments_,
			const wpp::View& identifier_,
			size_t n_popped_args_
		):
			arguments(arguments_),
			identifier(identifier_),
			n_popped_args(n_popped_args_) {}

		Pop() {}
	};

	struct Ctx {
		wpp::node_t expr{};

		Ctx(const wpp::node_t expr_): expr(expr_) {}
		Ctx() {}
	};

	struct Slice {
		wpp::node_t expr{};

		int start{};
		int stop{};

		enum {
			SLICE_INDEX = 0b0000'0001,
			SLICE_START = 0b0000'0010,
			SLICE_STOP  = 0b0000'0100,
		};

		uint8_t set;

		Slice(const wpp::node_t expr_, int start_, int stop_):
			expr(expr_), start(start_), stop(stop_) {}

		Slice() {}
	};

	// An alias for our AST.
	using AST = wpp::HeterogenousVector<
		IntrinsicUse,
		IntrinsicFile,
		IntrinsicPipe,
		IntrinsicRun,
		IntrinsicError,
		IntrinsicLog,
		IntrinsicAssert,
		Ctx,
		Slice,
		Pop,
		FnInvoke,
		Fn,
		VarRef,
		Var,
		Codeify,
		Match,
		String,
		Concat,
		Block,
		Document,
		Drop
	>;
}

#endif
