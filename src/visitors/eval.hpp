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


	inline std::string intrinsic_log(wpp::node_t expr, const wpp::Position&, wpp::Environment& env, wpp::Arguments* args = nullptr) {
		std::string str = eval_ast(expr, env, args);
		std::cerr << str;
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

	inline std::string intrinsic_slice(
		wpp::node_t string_expr,
		wpp::node_t start_expr,
		wpp::node_t end_expr,
		const wpp::Position& pos,
		wpp::Environment& env,
		wpp::Arguments* args = nullptr
	) {
		// Evaluate arguments
		const auto string = eval_ast(string_expr, env, args);
		const auto start_raw = eval_ast(start_expr, env, args);
		const auto end_raw = eval_ast(end_expr, env, args);

		// Parse the start and end arguments
		int start;
		int end;

		try {
			start = std::stoi(start_raw);
			end = std::stoi(end_raw);
		}

		catch (...) {
			throw wpp::Exception { pos, "slice range must be numerical." };
		}

		const int len = string.length();

		// Work out the start and length of the slice
		int begin;
		int count;

		if (start < 0)
			begin = len + start;
		else
			begin = start;

		if (end < 0)
			count = (len + end) - begin + 1;
		else
			count = end - begin + 1;

		// Make sure the range is valid
		if (count <= 0)
			throw wpp::Exception{ pos, "end of slice cannot be before the start." };

		else if (len < begin + count)
			throw wpp::Exception{ pos, "slice extends outside of string bounds." };

		else if (start < 0 && end >= 0)
			throw wpp::Exception{ pos, "start cannot be negative where end is positive." };

		// Return the string slice
		else
			return string.substr(begin, count);
	}

	inline std::string intrinsic_find(
		wpp::node_t string_expr,
		wpp::node_t pattern_expr,
		wpp::Environment& env,
		wpp::Arguments* args = nullptr
	) {
		// Evaluate arguments
		const auto string = eval_ast(string_expr, env, args);
		const auto pattern = eval_ast(pattern_expr, env, args);

		// Search in string
		auto position = string.find(pattern);

		if (position != std::string::npos)
			return std::to_string(position);
		else
			return "";
	}

	inline std::string intrinsic_length(wpp::node_t string_expr, wpp::Environment& env, wpp::Arguments* args) {
		// Evaluate argument
		const auto string = eval_ast(string_expr, env, args);

		return std::to_string(string.size());
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

		int rc = 0;
		std::string out = wpp::exec(runner, rc);

		// trim trailing newline.
		if (out.back() == '\n')
			out.erase(out.end() - 1, out.end());

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
					std::array<size_t, TOKEN_TOTAL> lookup{};

					lookup[TOKEN_SLICE]  = 3;
					lookup[TOKEN_FIND]   = 2;
					lookup[TOKEN_ASSERT] = 2;
					lookup[TOKEN_PIPE]   = 2;
					lookup[TOKEN_ERROR]  = 1;
					lookup[TOKEN_FILE]   = 1;
					lookup[TOKEN_ESCAPE] = 1;
					lookup[TOKEN_EVAL]   = 1;
					lookup[TOKEN_RUN]    = 1;
					lookup[TOKEN_SOURCE] = 1;
					lookup[TOKEN_LENGTH] = 1;
					lookup[TOKEN_LOG]    = 1;

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

				else if (type == TOKEN_SLICE)
					str = wpp::intrinsic_slice(exprs[0], exprs[1], exprs[2], pos, env, args);

				else if (type == TOKEN_FIND)
					str = wpp::intrinsic_find(exprs[0], exprs[1], env, args);

				else if (type == TOKEN_LENGTH)
					str = wpp::intrinsic_length(exprs[0], env, args);

				else if (type == TOKEN_LOG)
					str = wpp::intrinsic_log(exprs[0], pos, env, args);
			},

			[&] (const FnInvoke& call) {
				const auto& [caller_name, caller_args, caller_pos] = call;
				std::string caller_mangled_name = wpp::cat(caller_name, caller_args.size());

				// Check if parameter.
				if (args) {
					if (auto it = (*args).find(caller_name); it != (*args).end()) {
						if (caller_args.size() > 0)
							throw wpp::Exception{caller_pos, "calling '", caller_name, "' as if it were a function, it is an argument."};

						str = it->second;

						// Check if it's shadowing a function (even this one).
						auto func = functions.find(it->first + "0");
						wpp::warn(caller_pos, "parameter ", it->first, " is shadowing function ", func->first);

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

			[&] (const Map& map) {
				const auto& [test, cases, default_case, pos] = map;

				const auto test_str = eval_ast(test, env, args);

				// Compare test_str with arms of the map.
				auto it = std::find_if(cases.begin(), cases.end(), [&] (const auto& elem) {
					return test_str == eval_ast(elem.first, env, args);
				});

				// If found, evaluate the hand.
				if (it != cases.end())
					str = eval_ast(it->second, env, args);

				// If not found, check for a default arm, otherwise error.
				else {
					if (default_case == wpp::NODE_EMPTY)
						throw wpp::Exception{pos, "no matches found."};

					else
						str = eval_ast(default_case, env, args);
				}
			},

			[&] (const Pre& pre) {
				const auto& [name, stmts, pos] = pre;

				for (const wpp::node_t stmt: stmts) {
					if (wpp::Fn* func = std::get_if<wpp::Fn>(&tree[stmt])) {
						func->identifier = name + func->identifier;
						str += eval_ast(stmt, env, args);
					}

					else if (wpp::Pre* pre = std::get_if<wpp::Pre>(&tree[stmt])) {
						pre->identifier = name + pre->identifier;
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

