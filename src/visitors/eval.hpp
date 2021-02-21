#pragma once

#ifndef WOTPP_EVAL
#define WOTPP_EVAL

#include <string>
#include <unordered_map>
#include <filesystem>
#include <array>
#include <type_traits>
#include <limits>
#include <numeric>
#include <optional>

#include <utils/util.hpp>
#include <structures/ast.hpp>
#include <exception.hpp>
#include <parser.hpp>
#include <visitors/reconstruct.hpp>

// AST visitor that evaluates the program.

namespace wpp {
	using Arguments = std::unordered_map<std::string, std::string>;

	struct Caller {
		const wpp::node_t& func;
		bool conditional = false;

		Caller(const wpp::node_t& func_):
			func(func_) { }
	};

	struct FnEnvironment {
		const Caller& caller;
		const Arguments& args;

		FnEnvironment(
			const Caller& caller_,
			const Arguments& args_
		):
			caller(caller_),
			args(args_) {}
	};

	struct Environment {
		std::unordered_map<std::string, std::vector<wpp::node_t>> functions{};
		wpp::AST& tree;

		Environment(wpp::AST& tree_): tree(tree_) {}
	};

	inline std::string eval_ast(const wpp::node_t, wpp::Environment&, const std::optional<wpp::FnEnvironment>&);


	inline std::string intrinsic_assert(
		wpp::node_t node_id,
		wpp::node_t a, wpp::node_t b,
		const wpp::Position& pos,
		wpp::Environment& env,
		const std::optional<wpp::FnEnvironment>& fn_env
	) {
		auto& [functions, tree] = env;

		// Check if strings are equal.
		const auto str_a = eval_ast(a, env, fn_env);
		const auto str_b = eval_ast(b, env, fn_env);

		if (str_a != str_b)
			throw wpp::Exception{
				pos, "assertion failed: ", reconstruct_source(node_id, tree)
			};

		return "";
	}


	inline std::string intrinsic_error(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, const std::optional<wpp::FnEnvironment>& fn_env) {
		const auto msg = eval_ast(expr, env, fn_env);
		throw wpp::Exception{ pos, msg };
		return "";
	}


	inline std::string intrinsic_file(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, const std::optional<wpp::FnEnvironment>& fn_env) {
		const auto fname = eval_ast(expr, env, fn_env);

		try {
			return wpp::read_file(fname);
		}

		catch (...) {
			throw wpp::Exception{ pos, "failed reading file '", fname, "'" };
		}
	}


	inline std::string intrinsic_source(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, const std::optional<wpp::FnEnvironment>& fn_env) {
		const auto fname = eval_ast(expr, env, fn_env);

		const auto old_path = std::filesystem::current_path();
		const auto new_path = old_path / std::filesystem::path{fname};

		std::string file;

		try {
			file = wpp::read_file(fname);
		}

		catch (const std::filesystem::filesystem_error& e) {
			throw wpp::Exception{pos, "file '", fname, "' not found."};
		}

		std::filesystem::current_path(new_path.parent_path());

		wpp::Lexer lex{new_path.string(), file.c_str()};
		wpp::node_t root;

		try {
			root = document(lex, env.tree);
			return wpp::eval_ast(root, env, fn_env);
		}

		catch (const wpp::Exception& e) {
			throw wpp::Exception{ pos, e.what() };
		}

		std::filesystem::current_path(old_path);

		return "";
	}


	inline std::string intrinsic_log(wpp::node_t expr, const wpp::Position&, wpp::Environment& env, const std::optional<wpp::FnEnvironment>& fn_env) {
		std::string str = eval_ast(expr, env, fn_env);
		std::cerr << str;
		return "";
	}


	inline std::string intrinsic_escape(wpp::node_t expr, const wpp::Position&, wpp::Environment& env, const std::optional<wpp::FnEnvironment>& fn_env) {
		std::string str;
		const auto input = eval_ast(expr, env, fn_env);

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
		const std::optional<wpp::FnEnvironment>& fn_env
	) {
		// Evaluate arguments
		const auto string = eval_ast(string_expr, env, fn_env);
		const auto start_raw = eval_ast(start_expr, env, fn_env);
		const auto end_raw = eval_ast(end_expr, env, fn_env);

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
		const std::optional<wpp::FnEnvironment>& fn_env
	) {
		// Evaluate arguments
		const auto string = eval_ast(string_expr, env, fn_env);
		const auto pattern = eval_ast(pattern_expr, env, fn_env);

		// Search in string
		auto position = string.find(pattern);

		if (position != std::string::npos)
			return std::to_string(position);
		else
			return "";
	}

	inline std::string intrinsic_length(wpp::node_t string_expr, wpp::Environment& env, const std::optional<wpp::FnEnvironment>& fn_env) {
		// Evaluate argument
		const auto string = eval_ast(string_expr, env, fn_env);

		return std::to_string(string.size());
	}

	inline std::string intrinsic_eval(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, const std::optional<wpp::FnEnvironment>& fn_env) {
		auto& [functions, tree] = env;

		const auto code = eval_ast(expr, env, fn_env);

		wpp::Lexer lex{"<eval>", code.c_str()};
		wpp::node_t root;

		try {
			root = document(lex, tree);
			return wpp::eval_ast(root, env, fn_env);
		}

		catch (const wpp::Exception& e) {
			throw wpp::Exception{ pos, "inside eval: ", e.what() };
		}
	}


	inline std::string intrinsic_run(wpp::node_t expr, const wpp::Position& pos, wpp::Environment& env, const std::optional<wpp::FnEnvironment>& fn_env) {
		const auto cmd = eval_ast(expr, env, fn_env);

		int rc = 0;
		std::string str = wpp::exec(cmd, rc);

		// trim trailing newline.
		if (str.back() == '\n')
			str.erase(str.end() - 1, str.end());

		if (rc)
			throw wpp::Exception{ pos, "subprocess exited with non-zero status." };

		return str;
	}


	inline std::string intrinsic_pipe(wpp::node_t cmd, wpp::node_t data, const wpp::Position& pos, wpp::Environment& env, const std::optional<wpp::FnEnvironment>& fn_env) {
		std::string str;

		const auto cmd_str = eval_ast(cmd, env, fn_env);
		const auto data_str = eval_ast(data, env, fn_env);

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


	inline std::string eval_ast(const wpp::node_t node_id, wpp::Environment& env, const std::optional<wpp::FnEnvironment>& fn_env) {
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
					str = wpp::intrinsic_assert(node_id, exprs[0], exprs[1], pos, env, fn_env);

				else if (type == TOKEN_ERROR)
					str = wpp::intrinsic_error(exprs[0], pos, env, fn_env);

				else if (type == TOKEN_FILE)
					str = wpp::intrinsic_file(exprs[0], pos, env, fn_env);

				else if (type == TOKEN_SOURCE)
					str = wpp::intrinsic_source(exprs[0], pos, env, fn_env);

				else if (type == TOKEN_ESCAPE)
					str = wpp::intrinsic_escape(exprs[0], pos, env, fn_env);

				else if (type == TOKEN_EVAL)
					str = wpp::intrinsic_eval(exprs[0], pos, env, fn_env);

				else if (type == TOKEN_RUN)
					str = wpp::intrinsic_run(exprs[0], pos, env, fn_env);

				else if (type == TOKEN_PIPE)
					str = wpp::intrinsic_pipe(exprs[0], exprs[1], pos, env, fn_env);

				else if (type == TOKEN_SLICE)
					str = wpp::intrinsic_slice(exprs[0], exprs[1], exprs[2], pos, env, fn_env);

				else if (type == TOKEN_FIND)
					str = wpp::intrinsic_find(exprs[0], exprs[1], env, fn_env);

				else if (type == TOKEN_LENGTH)
					str = wpp::intrinsic_length(exprs[0], env, fn_env);

				else if (type == TOKEN_LOG)
					str = wpp::intrinsic_log(exprs[0], pos, env, fn_env);
			},

			[&] (const FnInvoke& call) {
				const auto& [call_name, call_args, call_pos] = call;
				std::string call_mangled_name = wpp::cat(call_name, call_args.size());

				// Check if parameter.
				if (fn_env) {
					auto& [caller, args] = fn_env.value();

					if (auto it = args.find(call_name); it != args.end()) {
						if (call_args.size() > 0)
							throw wpp::Exception{call_pos, "calling argument '", call_name, "' as if it were a function."};

						str = it->second;

						// Check if it's shadowing a function (even this one).
						if (functions.find(wpp::cat(it->first, "0")) != functions.end())
							wpp::warn(call_pos, "parameter ", call_name, " is shadowing a function.");

						return;
					}
				}

				// If it wasn't a parameter, we fall through to here and check if it's a function.
				auto it = functions.find(call_mangled_name);
				if (it == functions.end() or it->second.empty())
					throw wpp::Exception{call_pos, "func not found: ", call_name, "."};

				const auto func = tree.get<wpp::Fn>(it->second.back());

				// Retrieve function.
				const auto& [func_name, params, body, func_pos] = func;

				// Set up Arguments to pass down to function body.
				Arguments env_args;
				if (fn_env) {
					auto& [caller, args] = fn_env.value();
					for (const auto& [key, val]: args)
						env_args.emplace(key, val);
				}

				// Evaluate arguments and store their result.
				for (int i = 0; i < (int)call_args.size(); i++)
					env_args.insert_or_assign(params[i], eval_ast(call_args[i], env, fn_env));

				// Setup ourselves as the caller
				Caller env_caller(func.body);

				// If we have been called from a function.
				if (fn_env) {
					auto& [caller, args] = fn_env.value();

					// Check if we are unconditionally calling ourselves (which should result in
					// infinite recursion, and ultimately, in segfault), and throw a warning if
					// we are.
					// See: https://github.com/Jackojc/wotpp/issues/4#issuecomment-782187295

					// We pass the check if and only if the function node ids are the same,
					// we are not inside a map block, and all the arguments match.

					if (caller.func == body and (not caller.conditional)) {
						// Check arguments
						bool args_identical = true;
						for (const auto& [name, value]: args) {
							if (env_args[name] != value) {
								args_identical = false;
								break;
							}
						}

						if (args_identical)
							throw wpp::Exception(call_pos, "function ", func_name, " is trying to unconditionally call itself, which would result in infinite recursion.");
					}
				}

				// Setup an FnEnvironment
				const FnEnvironment fn_environment(env_caller, env_args);

				// Call function.
				str = eval_ast(body, env, fn_environment);
			},

			[&] (const Fn& func) {
				const auto& [name, params, body, pos] = func;

				auto it = functions.find(wpp::cat(name, params.size()));

				if (it != functions.end()) {
					wpp::warn(pos, "function '", name, "' redefined.");
					it->second.emplace_back(node_id);
				}

				else
					functions.emplace(wpp::cat(name, params.size()), std::vector{node_id});
			},

			[&] (const Drop& drop) {
				const auto& [func, pos] = drop;
				const auto& [caller_name, caller_args, caller_pos] = tree.get<FnInvoke>(func);

				std::string caller_mangled_name = wpp::cat(caller_name, caller_args.size());

				auto it = functions.find(caller_mangled_name);

				if (it != functions.end()) {
					if (not it->second.empty())
						it->second.pop_back();

					else
						functions.erase(it);
				}

				else {
					throw wpp::Exception{pos, "cannot drop undefined function '", caller_name, "' (", caller_args.size(), " parameters)."};
				}
			},

			[&] (const String& x) {
				str = x.value;
			},

			[&] (const Concat& cat) {
				const auto& [lhs, rhs, pos] = cat;
				str = eval_ast(lhs, env, fn_env) + eval_ast(rhs, env, fn_env);
			},

			[&] (const Block& block) {
				const auto& [stmts, expr, pos] = block;

				for (const wpp::node_t node: stmts)
					str += eval_ast(node, env, fn_env);

				str = eval_ast(expr, env, fn_env);
			},

			[&] (const Map& map) {
				const auto& [test, cases, default_case, pos] = map;

				const auto test_str = eval_ast(test, env, fn_env);

				// Compare test_str with arms of the map.
				auto it = std::find_if(cases.begin(), cases.end(), [&] (const auto& elem) {
					return test_str == eval_ast(elem.first, env, fn_env);
				});

				auto eval_hands = [&env, &it, &cases, &default_case, &pos, &str](auto& fn_env) {
					// If found, evaluate the hand.
					if (it != cases.end())
						str = eval_ast(it->second, env, fn_env);

					// If not found, check for a default arm, otherwise error.
					else {
						if (default_case == wpp::NODE_EMPTY)
							throw wpp::Exception{pos, "no matches found."};

						else
							str = eval_ast(default_case, env, fn_env);
					}
				};

				// If we have been called from inside function.
				if (fn_env) {
					auto& [caller, args] = fn_env.value();

					Caller env_caller(caller.func);
					env_caller.conditional = true;

					const FnEnvironment fn_environment(env_caller, args);
					eval_hands(fn_environment);
				}

				// If we have been called from the document root.
				else {
					eval_hands(fn_env);
				}
			},

			[&] (const Pre& pre) {
				const auto& [exprs, stmts, pos] = pre;

				for (const wpp::node_t stmt: stmts) {
					if (wpp::Fn* func = std::get_if<wpp::Fn>(&tree[stmt])) {
						std::string name;

						for (auto it = exprs.rbegin(); it != exprs.rend(); ++it)
							name += eval_ast(*it, env, fn_env);

						func->identifier = name + func->identifier;
						str += eval_ast(stmt, env, fn_env);
					}

					else if (wpp::Pre* pre = std::get_if<wpp::Pre>(&tree[stmt])) {
						pre->exprs.insert(pre->exprs.end(), exprs.begin(), exprs.end());
						str += eval_ast(stmt, env, fn_env);
					}

					else {
						str += eval_ast(stmt, env, fn_env);
					}
				}
			},

			[&] (const Document& doc) {
				for (const wpp::node_t node: doc.stmts)
					str += eval_ast(node, env, fn_env);
			}
		);

		return str;
	}
}

namespace wpp {
	int run(const std::string& fname) {
		std::string file;

		try {
			file = wpp::read_file(fname);
		}

		catch (const std::filesystem::filesystem_error& e) {
			tinge::errorln("file not found.");
			return 1;
		}

		// Set current path to path of file.
		std::filesystem::current_path(std::filesystem::current_path() / std::filesystem::path{fname}.parent_path());

		try {
			wpp::Lexer lex{fname, file.c_str()};
			wpp::AST tree;
			wpp::Environment env{tree};
			std::optional<wpp::FnEnvironment> root_env;

			tree.reserve((1024 * 1024 * 10) / sizeof(decltype(tree)::value_type));

			auto root = wpp::document(lex, tree);
			std::cout << wpp::eval_ast(root, env, root_env) << std::flush;
		}

		catch (const wpp::Exception& e) {
			wpp::error(e.pos, e.what());
			return 1;
		}

		return 0;
	}
}

#endif

