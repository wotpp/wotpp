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

				int rc = 0;
				str = wpp::exec(cmd, rc);

				// trim trailing newline.
				if (str.back() == '\n')
					str.erase(str.end() - 1);

				if (rc)
					throw wpp::Exception{ pos, "subprocess exited with non-zero status." };
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
				const auto& [nodes, pos] = ass;

				// Check if strings are equal.
				auto a = eval_ast(nodes.first, tree, functions, args);
				auto b = eval_ast(nodes.second, tree, functions, args);

				// todo: print reconstruction of AST nodes for arguments.
				// `assert(fun(x), "d")`
				// `assertion failure fun("a") != "d"`
				if (a != b)
					throw wpp::Exception{ pos, "assertion failed." };
			},

			[&] (const FnFile& file) {
				const auto& [arg, pos] = file;

				auto fname = eval_ast(arg, tree, functions, args);

				try {
					str = wpp::read_file(fname);
				}

				catch (...) {
					throw wpp::Exception{ pos, "failed reading file '", fname, "'" };
				}
			},

			[&] (const FnError& err) {
				const auto& [arg, pos] = err;

				auto msg = eval_ast(arg, tree, functions, args);
				throw wpp::Exception{ pos, msg };
			},

			[&] (const FnPipe& pipe) {
				const auto& [arg, pos] = pipe;
			},

			[&] (const FnInvoke& call) {
				const auto& [caller_name, caller_args, caller_pos] = call;
				std::string caller_mangled_name = cat(caller_name, caller_args.size());

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

			[&] (const Pre& pre) {
				const auto& [name, stmts, pos] = pre;

				for (const wpp::node_t stmt: stmts) {
					if (wpp::Fn* func = std::get_if<wpp::Fn>(&tree[stmt])) {
						func->identifier = name + func->identifier;
						str += eval_ast(stmt, tree, functions, args);
					}

					else {
						str += eval_ast(stmt, tree, functions, args);
					}
				}
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

