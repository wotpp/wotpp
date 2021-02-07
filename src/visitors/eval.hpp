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
			[&] (const FnInvoke& call) {
				const auto& [caller_name, caller_args, caller_pos] = call;
				std::string caller_mangled_name = mangle(caller_name, caller_args.size());

				// Check if parameter.
				if (args != nullptr) {
					if (auto man_it = (*args).find(caller_name); man_it != (*args).end()) {
						str = man_it->second;
						return;
					}
				}

				// Function.
				if (caller_name == "run") {
					// Set up arguments in environment.
					std::string command;

					for (int i = 0; i < (int)caller_args.size(); i++) {
						auto retstr = eval_ast(caller_args[i], tree, functions, args);
						command += retstr + " ";
					}

					// Run the command
					str = wpp::exec(command);
				}

				else {
					// If it wasn't a parameter, we fall through to here and check if it's a function.
					auto it = functions.find(caller_mangled_name);
					if (it == functions.end()) {
						wpp::error(caller_pos, "func not found: ", caller_name);
					}

					// Use function that was looked up.
					const auto& [callee_name, params, body, callee_pos] = tree.get<wpp::Fn>(it->second);

					// Set up arguments in environment.
					Arguments env_args;

					for (int i = 0; i < (int)caller_args.size(); i++) {
						auto retstr = eval_ast(caller_args[i], tree, functions, args);
						env_args.emplace(params[i], retstr);
					}
					// Call function.
					str = eval_ast(body, tree, functions, &env_args);
				}
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

