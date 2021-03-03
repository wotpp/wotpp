#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <array>
#include <type_traits>
#include <limits>
#include <numeric>
#include <algorithm>

#include <misc/util/util.hpp>
#include <misc/warnings.hpp>
#include <frontend/ast.hpp>
#include <structures/environment.hpp>
#include <structures/context.hpp>
#include <frontend/parser/parser.hpp>
#include <frontend/parser/ast_nodes.hpp>

#include <backend/eval/eval.hpp>
#include <backend/eval/intrinsics.hpp>


namespace wpp {
	std::string intrinsic_eval(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		auto& [ctx, ast, functions, positions, warnings] = env;

		const std::string source = wpp::evaluate(exprs[0], env, fn_env);

		wpp::Context new_ctx{ ctx.root, ctx.file, "eval", source.c_str() };
		wpp::Lexer lex{ new_ctx };

		return wpp::evaluate(wpp::parse(lex, env), env, fn_env);
	}


	std::string intrinsic_run(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		auto& [root, ast, functions, positions, warnings] = env;

		#if defined(WPP_DISABLE_RUN)
			wpp::error(positions.at(node_id), "run not available.");
		#endif

		const auto cmd = wpp::evaluate(exprs[0], env, fn_env);

		int rc = 0;
		std::string str = wpp::exec(cmd, rc);

		// trim trailing newline.
		if (str.back() == '\n')
			str.erase(str.end() - 1, str.end());

		if (rc)
			wpp::error(positions.at(node_id), "subprocess exited with non-zero status.");

		return str;
	}


	std::string intrinsic_pipe(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		auto& [root, ast, functions, positions, warnings] = env;

		#if defined(WPP_DISABLE_RUN)
			wpp::error(positions.at(node_id), "pipe not available.");
		#endif

		std::string str;

		const auto cmd = evaluate(exprs[0], env, fn_env);
		const auto data = evaluate(exprs[1], env, fn_env);

		int rc = 0;
		std::string out = wpp::exec(cmd, data, rc);

		// trim trailing newline.
		if (out.back() == '\n')
			out.erase(out.end() - 1, out.end());

		if (rc)
			wpp::error(positions.at(node_id), "subprocess exited with non-zero status.");

		return out;
	}


	std::string intrinsic_file(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		auto& [root, ast, functions, positions, warnings] = env;

		const auto fname = wpp::evaluate(exprs[0], env, fn_env);

		try {
			return wpp::read_file(std::filesystem::relative(std::filesystem::path{fname}));
		}

		catch (...) {
			wpp::error(positions.at(node_id), "failed reading file '", fname, "'");
		}
	}


	std::string intrinsic_source(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		auto& [ctx, ast, functions, positions, warnings] = env;

		std::string str;
		const auto fname = wpp::evaluate(exprs[0], env, fn_env);

		// Store current path and get the path of the new file.
		const auto old_path = std::filesystem::current_path();
		const auto new_path = old_path / std::filesystem::path{fname};

		std::filesystem::current_path(new_path.parent_path());

		try {
			const std::string source = wpp::read_file(new_path);

			wpp::Context new_ctx{ ctx.root, new_path, "normal", source.c_str() };
			wpp::Lexer lex{ new_ctx };

			str = wpp::evaluate(wpp::parse(lex, env), env, fn_env);

			std::filesystem::current_path(old_path);
		}

		catch (const std::filesystem::filesystem_error& e) {
			wpp::error(positions.at(node_id), "file '", fname, "' not found.");
		}

		return str;
	}


	std::string intrinsic_assert(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		auto& [root, ast, functions, positions, warnings] = env;

		// Check if strings are equal.
		const auto str_a = evaluate(exprs[0], env, fn_env);
		const auto str_b = evaluate(exprs[1], env, fn_env);

		if (str_a != str_b)
			wpp::error(positions.at(node_id), "assertion failed!");

		return "";
	}


	std::string intrinsic_error(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		auto& [root, ast, functions, positions, warnings] = env;

		const auto msg = evaluate(exprs[0], env, fn_env);
		wpp::error(positions.at(node_id), msg);

		return "";
	}


	std::string intrinsic_log(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		std::cerr << evaluate(exprs[0], env, fn_env);
		return "";
	}


	std::string intrinsic_escape(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		// Escape escape chars in a string.
		std::string str;

		const auto input = evaluate(exprs[0], env, fn_env);

		for (const char c: input) {
			switch (c) {
				case '"':  str += "\\\""; break;
				case '\'': str += "\\'";  break;
				case '\n': str += "\\n";  break;
				case '\t': str += "\\t";  break;
				case '\r': str += "\\r";  break;
				default:   str += c;      break;
			}
		}

		return str;
	}


	std::string intrinsic_slice(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		auto& [root, ast, functions, positions, warnings] = env;

		// Evaluate arguments
		const auto string = evaluate(exprs[0], env, fn_env);
		const auto start_raw = evaluate(exprs[1], env, fn_env);
		const auto end_raw = evaluate(exprs[2], env, fn_env);

		// Parse the start and end arguments
		int start = 0;
		int end = 0;


		try {
			start = std::stoi(start_raw);
			end = std::stoi(end_raw);
		}

		catch (...) {
			wpp::error(positions.at(node_id), "slice range must be numerical.");
		}


		const int len = string.length();

		// Work out the start and length of the slice
		int begin = 0;
		int count = 0;


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
			wpp::error(positions.at(node_id), "end of slice cannot be before the start.");

		else if (len < begin + count)
			wpp::error(positions.at(node_id), "slice extends outside of string bounds.");

		else if (start < 0 && end >= 0)
			wpp::error(positions.at(node_id), "start cannot be negative where end is positions.at(node_id)itive.");


		// Return the string slice
		return string.substr(begin, count);
	}


	std::string intrinsic_find(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		// Evaluate arguments
		const auto string = evaluate(exprs[0], env, fn_env);
		const auto pattern = evaluate(exprs[1], env, fn_env);

		// Search in string. Returns the index of a match.
		if (auto position = string.find(pattern); position != std::string::npos)
			return std::to_string(position);

		return "";
	}


	std::string intrinsic_length(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		// Evaluate argument
		const auto string = evaluate(exprs[0], env, fn_env);
		return std::to_string(string.size());
	}
}

