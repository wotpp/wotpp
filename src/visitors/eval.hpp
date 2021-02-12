#pragma once

#ifndef WOTPP_EVAL
#define WOTPP_EVAL

#include <string>
#include <unordered_map>
#include <filesystem>
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
	using Types = std::variant<std::string, std::vector<std::string>>;
	using VariadicArguments = std::vector<std::string>;

	inline std::string eval_ast(const wpp::node_t node_id, wpp::AST& tree, Environment& functions, Arguments* args = nullptr, VariadicArguments* varargs = nullptr) {
		const auto& variant = tree[node_id];
		std::string str;

		wpp::visit(variant,
			[&] (const Intrinsic& fn) {
				const auto& [type, exprs, pos] = fn;

				if (type == TOKEN_ASSERT) {
					if (exprs.size() != 2)
						throw wpp::Exception{pos, "assert takes exactly 2 arguments."};

					// Check if strings are equal.
					auto a = eval_ast(exprs[0], tree, functions, args);
					auto b = eval_ast(exprs[1], tree, functions, args);

					// todo: print reconstruction of AST nodes for arguments.
					// `assert(fun(x), "d")`
					// `assertion failure fun("a") != "d"`
					if (a != b)
						throw wpp::Exception{ pos, "assertion failed." };
				}

				else if (type == TOKEN_ERROR) {
					if (exprs.size() != 1)
						throw wpp::Exception{pos, "error takes exactly 1 argument."};

					auto msg = eval_ast(exprs[0], tree, functions, args);
					throw wpp::Exception{ pos, msg };
				}

				else if (type == TOKEN_FILE) {
					if (exprs.size() != 1)
						throw wpp::Exception{pos, "file takes exactly 1 argument."};

					auto fname = eval_ast(exprs[0], tree, functions, args);

					try {
						str = wpp::read_file(fname);
					}

					catch (...) {
						throw wpp::Exception{ pos, "failed reading file '", fname, "'" };
					}
				}

				else if (type == TOKEN_SOURCE) {
					if (exprs.size() != 1)
						throw wpp::Exception{pos, "source takes exactly 1 argument."};

					auto fname = eval_ast(exprs[0], tree, functions, args);

					throw wpp::Exception{ pos, "source not implemented." };

					// try {
					// 	str = wpp::read_file(fname);
					// }

					// catch (...) {
					// 	throw wpp::Exception{ pos, "failed sourcing '", fname, "'" };
					// }
				}

				else if (type == TOKEN_ESCAPE) {
					if (exprs.size() != 1)
						throw wpp::Exception{pos, "escape takes exactly 1 argument."};

					auto input = eval_ast(exprs[0], tree, functions, args);

					throw wpp::Exception{ pos, "escape not implemented." };
				}

				else if (type == TOKEN_EVAL) {
					if (exprs.size() != 1)
						throw wpp::Exception{pos, "eval takes exactly 1 argument."};

					auto code = eval_ast(exprs[0], tree, functions, args);

					wpp::Lexer lex{code.c_str()};
					wpp::node_t root;

					try {
						root = document(lex, tree);
						str = wpp::eval_ast(root, tree, functions, args);
					}

					catch (const wpp::Exception& e) {
						throw wpp::Exception{ pos, "inside eval: ", e.what() };
					}
				}

				else if (type == TOKEN_RUN) {
					if (exprs.size() != 1)
						throw wpp::Exception{pos, "run takes exactly 1 argument."};

					auto cmd = eval_ast(exprs[0], tree, functions, args);

					int rc = 0;
					str = wpp::exec(cmd, rc);

					// trim trailing newline.
					if (str.back() == '\n')
						str.erase(str.end() - 1);

					if (rc)
						throw wpp::Exception{ pos, "subprocess exited with non-zero status." };
				}
			},

			[&] (const FnInvoke& call) {
				const auto& [caller_name, caller_args, caller_variadic, caller_pos] = call;

				// Check if parameter.
				if (args) {
					if (auto it = (*args).find(caller_name); it != (*args).end()) {
						str = it->second;
						return;
					}
				}

				int total_arg_count = caller_args.size();
				if(caller_variadic) {
					total_arg_count += varargs->size();
				}

				std::string caller_mangled_name = wpp::mangle(caller_name, total_arg_count, false);

				// If it wasn't a parameter, we fall through to here and check if it's a function.
				auto it = functions.find(caller_mangled_name);
				if (it == functions.end()) {
					// Perhaps it is variadic?
					caller_mangled_name = wpp::mangle(caller_name, total_arg_count, true);
					it = functions.find(caller_mangled_name);

					// It wasn't.
					if (it == functions.end())
						throw wpp::Exception{caller_pos, "func not found: ", caller_name};

					// Otherwise, it is.
				}

				// Set up Arguments to pass down to function body.
				Arguments env_args;
				VariadicArguments env_varargs;

				// Retrieve function.
				const auto& [callee_name, params, callee_variadic, body, callee_pos] = tree.get<wpp::Fn>(it->second);

				// Evaluate arguments and store their result.
				for (int i = 0; i < total_arg_count; i++) {
					// This is an ordinary param of the function we're calling.
					if (i < (int)params.size()) {
						std::string s;

						// This is an ordinary param of this invocation.
						if (i < (int)caller_args.size())
							s = eval_ast(caller_args[i], tree, functions, args, varargs);

						// This is a variadic param of this invocation.
						// (here is the heart of the unpack.)
						else
							s = varargs->at(i - caller_args.size() /* i - argsize is the position of the arg relative to '*' */);

						env_args.emplace(params[i], s);
					}

					// This is a variadic param of the function we're calling (at or after *).
					else {
						std::string s = eval_ast(caller_args[i], tree, functions, args, varargs);
						env_varargs.emplace_back(s);
					}
				}

				// Call function.
				str = eval_ast(body, tree, functions, &env_args, &env_varargs);
			},

			[&] (const Fn& func) {
				const auto& [name, params, variadic, body, pos] = func;
				auto mangled_name = wpp::mangle(name, params.size(), variadic);
				functions.insert_or_assign(mangled_name, node_id);
			},

			[&] (const String& x) {
				str = x.value;
			},

			[&] (const Concat& cat) {
				const auto& [lhs, rhs, pos] = cat;
				str = eval_ast(lhs, tree, functions, args, varargs) + eval_ast(rhs, tree, functions, args, varargs);
			},

			[&] (const Block& block) {
				const auto& [stmts, expr, pos] = block;

				for (const wpp::node_t node: stmts) {
					str += eval_ast(node, tree, functions, args, varargs);
				}

				str = eval_ast(expr, tree, functions, args, varargs);
			},

			[&] (const Pre& pre) {
				const auto& [name, stmts, pos] = pre;

				for (const wpp::node_t stmt: stmts) {
					if (wpp::Fn* func = std::get_if<wpp::Fn>(&tree[stmt])) {
						func->identifier = name + func->identifier;
						str += eval_ast(stmt, tree, functions, args, varargs);
					}

					else {
						str += eval_ast(stmt, tree, functions, args, varargs);
					}
				}
			},

			[&] (const Document& doc) {
				for (const wpp::node_t node: doc.stmts) {
					str += eval_ast(node, tree, functions, args, varargs);
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

