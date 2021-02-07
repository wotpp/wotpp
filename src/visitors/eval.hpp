#pragma once

#ifndef WOTPP_EVAL
#define WOTPP_EVAL

#include <string>
#include <unordered_map>
#include <stack>

#include <utils/util.hpp>
#include <structures/ast.hpp>
#include <parser.hpp>

// AST visitor that evaluates the program.

namespace wpp {
	using Environment = std::unordered_map<std::string, wpp::Fn>;
	using Arguments = std::unordered_map<std::string, std::string>;


	template <typename... Ts>
	inline std::string mangle(const std::string& first, Ts&&... args) {
		using namespace std;
		const auto helper = [] (auto&& x) {
			return std::to_string(x) + "__";
		};

		return first + "__" + (helper(args) + ...);
	}


	template <typename T>
	inline std::string eval_ast(const T& variant, const wpp::AST& tree, Environment& functions, Arguments* args = nullptr) {
		std::string str;

		wpp::visit(variant,
			[&] (const FnInvoke& call) {
				const auto& [caller_name, caller_args, caller_pos] = call;
				std::string caller_mangled_name = mangle(caller_name, caller_args.size());

				// Parameter.
				if (args != nullptr) {
					auto man_it = (*args).find(caller_name);
					if (man_it != (*args).end()) {
						str = man_it->second;
						return;
					}

					// if its not an arg, maybe its a function?
				}

				// Function.
				auto it = functions.find(caller_mangled_name);
				if (it == functions.end()) {
					wpp::error(caller_pos, "func not found: ", caller_name);
				}

				// Use function that was looked up.
				const auto& [callee_name, params, body, callee_pos] = it->second;

				// Set up arguments in environment.
				Arguments env_args;

				for (int i = 0; i < (int)caller_args.size(); i++) {
					auto retstr = eval_ast(tree[caller_args[i]], tree, functions, args);
					env_args.emplace(params[i], retstr);
				}

				// Call function.
				str = eval_ast(tree[body], tree, functions, &env_args);
			},

			[&] (const Fn& func) {
				const auto& [name, params, body, pos] = func;
				functions.insert_or_assign(mangle(name, params.size()), func);
			},

			[&] (const String& x) {
				str = x.value;
			},

			[&] (const Concat& cat) {
				const auto& [lhs, rhs, pos] = cat;
				str = eval_ast(tree[lhs], tree, functions, args) + eval_ast(tree[rhs], tree, functions, args);
			},

			[&] (const Block& block) {
				const auto& [stmts, expr, pos] = block;

				for (const wpp::node_t node: stmts) {
					str += eval_ast(tree[node], tree, functions, args);
				}

				str = eval_ast(tree[expr], tree, functions, args);
			},

			[&] (const Ns&) {
				// const auto& [name, stmts] = ns;
			},

			[&] (const Document& doc) {
				for (const wpp::node_t node: doc.exprs_or_stmts) {
					str += eval_ast(tree[node], tree, functions, args);
				}
			}
		);

		return str;
	}
}

#endif

