#pragma once

#ifndef WOTPP_EVAL
#define WOTPP_EVAL

#include <string>
#include <unordered_map>
#include <stack>
#include <type_traits>

#include <utils/util.hpp>
#include <structures/ast.hpp>
#include <parser.hpp>

// AST visitor that evaluates the program.

namespace wpp {
	using Environment = std::unordered_map<std::string, wpp::node_t>;
	using Arguments = std::unordered_map<std::string, std::string>;


	template <typename... Ts>
	inline std::string mangle(Ts&&... args) {
		const auto tostr = [] (const auto& x) {
			if constexpr(std::is_same_v<std::decay_t<decltype(x)>, std::string>)
				return x;

			else
				return std::to_string(x);
		};

		return (tostr(args) + ...);
	}


	inline std::string eval_ast(const wpp::node_t node_id, const wpp::AST& tree, Environment& functions, Arguments* args = nullptr) {
		const auto& variant = tree[node_id];
		std::string str;

		wpp::visit(variant,
			[&] (const FnRun& run) {
				const auto& [arg, pos] = run;

				auto cmd = eval_ast(arg, tree, functions, args);
				str = wpp::exec(cmd);
			},

			[&] (const FnEval& eval) {
				const auto& [arg, pos] = eval;
			},

			[&] (const FnAssert& ass) {
				const auto& [arg, pos] = ass;
			},

			[&] (const FnFile& file) {
				const auto& [arg, pos] = file;
			},

			[&] (const FnInvoke& call) {
				const auto& [caller_name, caller_args, caller_pos] = call;
				std::string caller_mangled_name = mangle(caller_name, caller_args.size());

				// Check if parameter.
				if (args) {
					if (auto it = (*args).find(caller_name); it != (*args).end()) {
						str = it->second;
						return;
					}
				}

				// If it wasn't a parameter, we fall through to here and check if it's a function.
				auto it = functions.find(caller_mangled_name);
				if (it == functions.end())
					wpp::error(caller_pos, "func not found: ", caller_name);

				// Retrieve function.
				const auto& [callee_name, params, body, callee_pos] = tree.get<wpp::Fn>(it->second);

				// Set up Arguments to pass down to function body.
				Arguments env_args;

				// Evaluate arguments and store their result.
				for (int i = 0; i < (int)caller_args.size(); i++)
					env_args.emplace(params[i], eval_ast(caller_args[i], tree, functions, args));

				// Call function.
				str = eval_ast(body, tree, functions, &env_args);
			},

			[&] (const Fn& func) {
				const auto& [name, params, body, pos] = func;
				functions.insert_or_assign(mangle(name, params.size()), node_id);
			},

			[&] (const String& x) {
				str = x.value;
			},

			[&] (const Concat& cat) {
				const auto& [lhs, rhs, pos] = cat;
				str = eval_ast(lhs, tree, functions, args) + eval_ast(rhs, tree, functions, args);
			},

			[&] (const Block& block) {
				const auto& [stmts, expr, pos] = block;

				for (const wpp::node_t node: stmts) {
					str += eval_ast(node, tree, functions, args);
				}

				str = eval_ast(expr, tree, functions, args);
			},

			[&] (const Ns&) {
				// const auto& [name, stmts] = ns;
			},

			[&] (const Document& doc) {
				for (const wpp::node_t node: doc.exprs_or_stmts) {
					str += eval_ast(node, tree, functions, args);
				}
			}
		);

		return str;
	}
}

#endif

