#pragma once

#ifndef WOTPP_EVAL
#define WOTPP_EVAL

#include <string>
#include <unordered_map>
#include <stack>
#include <type_traits>

#include <utils/util.hpp>
#include <structures/ast.hpp>
#include <exception.hpp>
#include <parser.hpp>

// AST visitor that evaluates the program.

namespace wpp {
	using Environment = std::unordered_map<std::string, wpp::node_t>;
	using Arguments = std::unordered_map<std::string, std::string>;


	// TODO: Separate this into a header/implementation file so we don't have to forward declare this
	inline std::string eval(const std::string& code, wpp::Environment& env);

	inline std::string eval_ast(const wpp::node_t node_id, wpp::AST& tree, Environment& functions, Arguments* args = nullptr) {
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

				auto code = eval_ast(arg, tree, functions, args);

				wpp::Lexer lex{code.c_str()};
				wpp::node_t root;

				try {
					root = document(lex, tree);
					str = wpp::eval_ast(root, tree, functions, args);
				}

				catch (const wpp::Exception& e) {
					throw wpp::Exception{ pos, "inside eval: ", e.what() };
				}
			},

			[&] (const FnAssert& ass) {
				const auto& [arg, pos] = ass;
			},

			[&] (const FnFile& file) {
				const auto& [arg, pos] = file;
			},

			[&] (const FnInvoke& call) {
				const auto& [caller_name, caller_args, caller_pos] = call;
				std::string caller_catd_name = cat(caller_name, caller_args.size());

				// Check if parameter.
				if (args) {
					if (auto it = (*args).find(caller_name); it != (*args).end()) {
						str = it->second;
						return;
					}
				}

				// If it wasn't a parameter, we fall through to here and check if it's a function.
				auto it = functions.find(caller_catd_name);
				if (it == functions.end())
					throw wpp::Exception{caller_pos, "func not found: ", caller_name};

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
				functions.insert_or_assign(cat(name, params.size()), node_id);
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
				for (const wpp::node_t node: doc.stmts) {
					str += eval_ast(node, tree, functions, args);
				}
			}
		);

		return str;
	}

	inline std::string eval(const std::string& code, wpp::Environment& env) {
		// Create a new lexer and syntax tree
		wpp::Lexer lex{code.c_str()};
		wpp::AST tree;

		// Reserve 10MiB
		tree.reserve((1024 * 1024 * 10) / sizeof(decltype(tree)::value_type));

		// Parse.
		auto root = document(lex, tree);

		// Evaluate.
		return wpp::eval_ast(root, tree, env);
	}

}

#endif

