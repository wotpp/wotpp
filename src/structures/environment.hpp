#pragma once

#ifndef WOTPP_ENV
#define WOTPP_ENV

#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

#include <structures/context.hpp>
#include <frontend/ast.hpp>
#include <frontend/parser/ast_nodes.hpp>
#include <misc/warnings.hpp>

namespace wpp {
	using Functions = std::unordered_map<std::string, std::vector<wpp::node_t>>;
	using Arguments = std::unordered_map<std::string, std::string>;
	using Positions = std::unordered_map<wpp::node_t, wpp::Pos>;

	struct FnEnv {
		wpp::Arguments args;
	};

	struct Env {
		const wpp::Context ctx;

		wpp::AST ast{};

		wpp::Functions functions{};
		wpp::Positions positions{};

		const wpp::warning_t warning_flags{};


		Env(const wpp::Context& ctx_, const wpp::warning_t warning_flags_):
			ctx(ctx_),
			warning_flags(warning_flags_)
		{
			ast.reserve(ast.capacity() + (1024 * 1024 * 10) / sizeof(decltype(ast)::value_type));
		}
	};
}

#endif
