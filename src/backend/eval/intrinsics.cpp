#include <string>
#include <vector>
#include <filesystem>

#include <misc/util/util.hpp>
#include <misc/flags.hpp>
#include <frontend/ast.hpp>
#include <structures/environment.hpp>
#include <frontend/parser/parser.hpp>
#include <backend/eval/eval.hpp>


namespace wpp {
	std::string intrinsic_eval(
		const wpp::node_t,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		const std::string source = wpp::evaluate(exprs[0], env, fn_env);

		const auto& [file, base, mode] = env.sources.top();
		env.sources.push(file, source, modes::eval);

		return wpp::evaluate(wpp::parse(env), env, fn_env);
	}


	std::string intrinsic_run(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		#if defined(WPP_DISABLE_RUN)
			wpp::error(node_id, env, "run not available");
		#endif

		if (env.flags & wpp::FLAG_DISABLE_RUN)
			wpp::error(node_id, env, "run not available");

		const auto cmd = wpp::evaluate(exprs[0], env, fn_env);

		int rc = 0;
		std::string str = wpp::exec(cmd, rc);

		// trim trailing newline.
		if (str.back() == '\n')
			str.erase(str.end() - 1, str.end());

		if (rc)
			wpp::error(node_id, env, "subprocess exited with non-zero status");

		return str;
	}


	std::string intrinsic_pipe(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		#if defined(WPP_DISABLE_RUN)
			wpp::error(node_id, env, "pipe not available");
		#endif

		if (env.flags & wpp::FLAG_DISABLE_RUN)
			wpp::error(node_id, env, "pipe not available");

		std::string str;

		const auto cmd = evaluate(exprs[0], env, fn_env);
		const auto data = evaluate(exprs[1], env, fn_env);

		int rc = 0;
		std::string out = wpp::exec(cmd, data, rc);

		// trim trailing newline.
		if (out.back() == '\n')
			out.erase(out.end() - 1, out.end());

		if (rc)
			wpp::error(node_id, env, "subprocess exited with non-zero status");

		return out;
	}


	std::string intrinsic_file(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		const auto fname = wpp::evaluate(exprs[0], env, fn_env);

		try {
			return wpp::read_file(std::filesystem::relative(std::filesystem::path{fname}));
		}

		catch (...) {
			wpp::error(node_id, env, "could not read file", wpp::cat("file '", fname, "' does not exist or could not be found"));
		}

		return "";
	}


	std::string intrinsic_source(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		std::string str;
		const auto fname = wpp::evaluate(exprs[0], env, fn_env);

		// Store current path and get the path of the new file.
		const auto old_path = std::filesystem::current_path();
		const auto new_path = old_path / std::filesystem::path{fname};

		std::filesystem::current_path(new_path.parent_path());

		try {
			const std::string source = wpp::read_file(new_path);

			env.sources.push(new_path, source, wpp::modes::source);
			str = wpp::evaluate(wpp::parse(env), env, fn_env);

			std::filesystem::current_path(old_path);
		}

		catch (const std::filesystem::filesystem_error& e) {
			wpp::error(node_id, env, "could not read file", wpp::cat("file '", fname, "' does not exist or could not be found"));
		}

		return str;
	}


	std::string intrinsic_assert(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		// Check if strings are equal.
		const auto str_a = evaluate(exprs[0], env, fn_env);
		const auto str_b = evaluate(exprs[1], env, fn_env);

		if (str_a != str_b)
			wpp::error(node_id, env, "assertion failed", wpp::cat("lhs='", str_a, "', rhs='", str_b, "'"));

		return "";
	}


	std::string intrinsic_error(
		const wpp::node_t node_id,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		const auto msg = evaluate(exprs[0], env, fn_env);
		wpp::error(node_id, env, msg);

		return "";
	}


	std::string intrinsic_log(
		const wpp::node_t,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		std::cerr << evaluate(exprs[0], env, fn_env);
		return "";
	}


	std::string intrinsic_escape(
		const wpp::node_t,
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
			wpp::error(node_id, env, "slice range must be numerical");
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
			wpp::error(node_id, env, "end of slice cannot be before the start");

		else if (len < begin + count)
			wpp::error(node_id, env, "slice extends outside of string bounds");

		else if (start < 0 && end >= 0)
			wpp::error(node_id, env, "start cannot be negative where end is negative");


		// Return the string slice
		return string.substr(begin, count);
	}


	std::string intrinsic_find(
		const wpp::node_t,
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
		const wpp::node_t,
		const std::vector<wpp::node_t>& exprs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		// Evaluate argument
		const auto string = evaluate(exprs[0], env, fn_env);
		return std::to_string(string.size());
	}
}

