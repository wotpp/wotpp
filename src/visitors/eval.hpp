#pragma once

#ifndef WOTPP_EVAL
#define WOTPP_EVAL

#include <string>
#include <unordered_map>
#include <filesystem>
#include <stack>
#include <array>
#include <type_traits>
#include <limits>

#include <utils/util.hpp>
#include <structures/ast.hpp>
#include <exception.hpp>
#include <parser.hpp>
#include <visitors/reconstruct.hpp>

// AST visitor that evaluates the program.

namespace wpp {
	using Arguments = std::unordered_map<std::string, std::string>;

	struct Environment {
		std::unordered_map<std::string, wpp::node_t> functions{};
		wpp::AST& tree;

		Environment(wpp::AST& tree_): tree(tree_) {}
	};


	inline std::string eval_ast(const wpp::node_t, wpp::Environment&, wpp::Arguments* = nullptr);


	inline std::string intrinsic_assert(
		wpp::node_t node_id,
		wpp::node_t a, wpp::node_t b,
		const wpp::Position& pos,
		wpp::Environment& env,
		wpp::Arguments* args = nullptr
	) {
		auto& [functions, tree] = env;

		// Check if strings are equal.
		const auto str_a = eval_ast(a, env, args);
		const auto str_b = eval_ast(b, env, args);

		if (str_a != str_b)
			throw wpp::Exception{
				pos, "assertion failed: ", reconstruct_source(node_id, tree)
			};

		return "";
	}


	inline std::string intrinsic_error(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, wpp::Arguments* args = nullptr) {
		const auto msg = eval_ast(expr, env, args);
		throw wpp::Exception{ pos, msg };
		return "";
	}


	inline std::string intrinsic_file(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, wpp::Arguments* args = nullptr) {
		const auto fname = eval_ast(expr, env, args);

		try {
			return wpp::read_file(fname);
		}

		catch (...) {
			throw wpp::Exception{ pos, "failed reading file '", fname, "'" };
		}
	}


	inline std::string intrinsic_source(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, wpp::Arguments* args = nullptr) {
		const auto fname = eval_ast(expr, env, args);
		throw wpp::Exception{ pos, "source not implemented." };

		return "";
	}


	inline std::string intrinsic_escape(wpp::node_t expr, const wpp::Position&, wpp::Environment& env, wpp::Arguments* args = nullptr) {
		std::string str;
		const auto input = eval_ast(expr, env, args);

		for (const char c: input) {
			switch (c) {
				case '"':  str += "\\\""; break;
				case '\'': str += "\\'"; break;
				case '\n': str += "\\n"; break;
				case '\t': str += "\\t"; break;
				case '\r': str += "\\r"; break;
				default:   str += c; break;
			}
		}

		return str;
	}


	inline std::string intrinsic_eval(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, wpp::Arguments* args = nullptr) {
		auto& [functions, tree] = env;

		const auto code = eval_ast(expr, env, args);

		wpp::Lexer lex{code.c_str()};
		wpp::node_t root;

		try {
			root = document(lex, tree);
			return wpp::eval_ast(root, env, args);
		}

		catch (const wpp::Exception& e) {
			throw wpp::Exception{ pos, "inside eval: ", e.what() };
		}
	}


	inline std::string intrinsic_run(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, wpp::Arguments* args = nullptr) {
		const auto cmd = eval_ast(expr, env, args);

		int rc = 0;
		std::string str = wpp::exec(cmd, rc);

		// trim trailing newline.
		if (str.back() == '\n')
			str.erase(str.end() - 1, str.end());

		if (rc)
			throw wpp::Exception{ pos, "subprocess exited with non-zero status." };

		return str;
	}


	inline std::string intrinsic_pipe(wpp::node_t cmd, wpp::node_t data, const wpp::Position& pos, wpp::Environment& env, wpp::Arguments* args = nullptr) {
		std::string str;

		const auto cmd_str = eval_ast(cmd, env, args);
		const auto data_str = eval_ast(data, env, args);

		for (const char c: data_str) {
			switch (c) {
				case '"':  str += "\\\""; break;
				case '\'': str += "\\'"; break;
				case '\n': str += "\\n"; break;
				case '\t': str += "\\t"; break;
				case '\r': str += "\\r"; break;
				default:   str += c; break;
			}
		}

		std::string runner = wpp::cat("echo \"", str, "\" | ", cmd_str);

		tinge::warnln(runner);

		int rc = 0;
		std::string out = wpp::exec(runner, rc);

		// trim trailing newline.
		if (str.back() == '\n')
			str.erase(str.end() - 1, str.end());

		if (rc)
			throw wpp::Exception{ pos, "subprocess exited with non-zero status." };

		return out;
	}


	inline std::string eval_ast(const wpp::node_t node_id, wpp::Environment& env, wpp::Arguments* args) {
		auto& [functions, tree] = env;
		const auto& variant = tree[node_id];
		std::string str;

		wpp::visit(variant,
			[&] (const Intrinsic& fn) {
				const auto& [type, name, exprs, pos] = fn;

				constexpr std::array intrinsic_arg_n = [&] {
					std::array<size_t, 100> lookup{};

					lookup[TOKEN_ASSERT] = 2;
					lookup[TOKEN_PIPE]   = 2;
					lookup[TOKEN_ERROR]  = 1;
					lookup[TOKEN_FILE]   = 1;
					lookup[TOKEN_ESCAPE] = 1;
					lookup[TOKEN_EVAL]   = 1;
					lookup[TOKEN_RUN]    = 1;
					lookup[TOKEN_SOURCE] = 1;

					return lookup;
				} ();


				const auto n_args = intrinsic_arg_n[type];
				if (n_args != exprs.size())
					throw wpp::Exception{pos, name, " takes exactly ", n_args, " arguments."};


				if (type == TOKEN_ASSERT)
					str = wpp::intrinsic_assert(node_id, exprs[0], exprs[1], pos, env, args);

				else if (type == TOKEN_ERROR)
					str = wpp::intrinsic_error(exprs[0], pos, env, args);

				else if (type == TOKEN_FILE)
					str = wpp::intrinsic_file(exprs[0], pos, env, args);

				else if (type == TOKEN_SOURCE)
					str = wpp::intrinsic_source(exprs[0], pos, env, args);

				else if (type == TOKEN_ESCAPE)
					str = wpp::intrinsic_escape(exprs[0], pos, env, args);

				else if (type == TOKEN_EVAL)
					str = wpp::intrinsic_eval(exprs[0], pos, env, args);

				else if (type == TOKEN_RUN)
					str = wpp::intrinsic_run(exprs[0], pos, env, args);

				else if (type == TOKEN_PIPE)
					str = wpp::intrinsic_pipe(exprs[0], exprs[1], pos, env, args);
			},

			[&] (const FnInvoke& call) {
				const auto& [caller_name, caller_args, caller_pos] = call;
				std::string caller_mangled_name = wpp::cat(caller_name, caller_args.size());

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

				if (args) {
					for (const auto& [key, val]: *args)
						env_args.emplace(key, val);
				}

				// Evaluate arguments and store their result.
				for (int i = 0; i < (int)caller_args.size(); i++)
					env_args.insert_or_assign(params[i], eval_ast(caller_args[i], env, args));

				// Call function.
				str = eval_ast(body, env, &env_args);
			},

			[&] (const Fn& func) {
				const auto& [name, params, body, pos] = func;
				functions.insert_or_assign(wpp::cat(name, params.size()), node_id);
			},

			[&] (const String& x) {
				str = x.value;
			},

			[&] (const Concat& cat) {
				const auto& [lhs, rhs, pos] = cat;
				str = eval_ast(lhs, env, args) + eval_ast(rhs, env, args);
			},

			[&] (const Block& block) {
				const auto& [stmts, expr, pos] = block;

				for (const wpp::node_t node: stmts)
					str += eval_ast(node, env, args);

				str = eval_ast(expr, env, args);
			},

			[&] (const Pre& pre) {
				const auto& [name, stmts, pos] = pre;

				for (const wpp::node_t stmt: stmts) {
					if (wpp::Fn* func = std::get_if<wpp::Fn>(&tree[stmt])) {
						func->identifier = name + func->identifier;
						str += eval_ast(stmt, env, args);
					}

					else {
						str += eval_ast(stmt, env, args);
					}
				}
			},

			[&] (const Document& doc) {
				for (const wpp::node_t node: doc.stmts)
					str += eval_ast(node, env, args);
			}
		);

		return str;
	}

	inline std::string eval(const std::string& code) {
		// Create a new lexer and syntax tree
		wpp::Lexer lex{code.c_str()};
		wpp::AST tree;
		wpp::Environment env{tree};

		// Reserve 10MiB
		tree.reserve((1024 * 1024 * 10) / sizeof(decltype(tree)::value_type));

		// Parse.
		auto root = document(lex, tree);

		// Evaluate.
		return wpp::eval_ast(root, env);
	}
}

#endif

