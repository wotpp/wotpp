// #pragma once

#ifndef WOTPP_EVAL
#define WOTPP_EVAL

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

#include <frontend/parser/ast_nodes.hpp>

// AST visitor that evaluates the program.

namespace wpp {
	using Arguments = std::unordered_map<std::string, std::string>;

	struct Environment {
		std::filesystem::path base;
		std::unordered_map<std::string, std::vector<wpp::node_t>> functions{};
		wpp::AST& tree;
		wpp::warning_t warning_flags = 0;

		Environment(
			const std::filesystem::path& base_,
			wpp::AST& tree_,
			const wpp::warning_t warning_flags_ = 0
		):
			base(base_),
			tree(tree_),
			warning_flags(warning_flags_) {}
	};


	std::string intrinsic_assert(
		wpp::node_t expr,
		wpp::node_t a, wpp::node_t b,
		const wpp::Position& pos,
		wpp::Environment& env,
		wpp::Arguments* args = nullptr
	);

	std::string intrinsic_slice(
		wpp::node_t string_expr,
		wpp::node_t start_expr,
		wpp::node_t end_expr,
		const wpp::Position& pos,
		wpp::Environment& env,
		wpp::Arguments* args = nullptr
	);

	std::string intrinsic_find(
		wpp::node_t string_expr,
		wpp::node_t pattern_expr,
		wpp::Environment& env,
		wpp::Arguments* args = nullptr
	);

	std::string eval_ast(const wpp::node_t, wpp::Environment&, wpp::Arguments* = nullptr);
	std::string intrinsic_error(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, wpp::Arguments* args = nullptr);
	std::string intrinsic_file(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, wpp::Arguments* args = nullptr);
	std::string intrinsic_source(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, wpp::Arguments* args = nullptr);
	std::string intrinsic_log(wpp::node_t expr, const wpp::Position&, wpp::Environment& env, wpp::Arguments* args = nullptr);
	std::string intrinsic_escape(wpp::node_t expr, const wpp::Position&, wpp::Environment& env, wpp::Arguments* args = nullptr);
	std::string intrinsic_length(wpp::node_t string_expr, wpp::Environment& env, wpp::Arguments* args);
	std::string intrinsic_eval(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, wpp::Arguments* args = nullptr);
	std::string intrinsic_run(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, wpp::Arguments* args = nullptr);
	std::string intrinsic_pipe(wpp::node_t cmd, wpp::node_t data, const wpp::Position& pos, wpp::Environment& env, wpp::Arguments* args = nullptr);
}

namespace wpp {
	int run(const std::string& fname, const wpp::warning_t warning_flags = 0);
}

#endif

