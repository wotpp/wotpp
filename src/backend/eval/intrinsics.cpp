#include <string>
#include <vector>
#include <filesystem>

#include <misc/dbg.hpp>
#include <misc/util/util.hpp>
#include <misc/flags.hpp>
#include <frontend/ast.hpp>
#include <structures/environment.hpp>
#include <frontend/parser/parser.hpp>
#include <backend/eval/eval.hpp>


namespace wpp {
	std::string intrinsic_eval(
		wpp::node_t node_id,
		wpp::node_t expr,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		DBG();

		const std::string source = wpp::evaluate(expr, env, fn_env);

		const auto& [file, base, mode] = env.sources.top();
		env.sources.push(file, source, modes::eval);

		std::string str;
		wpp::node_t root;

		const auto old_state = env.state;
		env.state |= wpp::ABORT_ERROR_RECOVERY;

		try {
			root = wpp::parse(env);
		}

		catch (const wpp::Report& e) {
			wpp::error(report_modes::syntax, node_id, env, e.overview, e.detail, e.suggestion);
		}

		try {
			str = wpp::evaluate(root, env, fn_env);
		}

		catch (const wpp::Report& e) {
			wpp::error(report_modes::semantic, node_id, env, e.overview, e.detail, e.suggestion);
		}

		env.state = old_state;

		return str;
	}


	std::string intrinsic_run(
		wpp::node_t node_id,
		wpp::node_t expr,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		DBG();

		#if defined(WPP_DISABLE_RUN)
			wpp::error(report_modes::semantic, node_id, env, "instrinsic disabled", "`run` not available");

		#else
			if (env.flags & wpp::FLAG_DISABLE_RUN)
				wpp::error(report_modes::semantic, node_id, env, "intrinsic disabled", "`run` not available");

			const auto cmd = wpp::evaluate(expr, env, fn_env);

			int rc = 0;
			std::string str = wpp::exec(cmd, rc);

			// trim trailing newline.
			if (str.back() == '\n')
				str.erase(str.end() - 1, str.end());

			if (rc)
				wpp::error(report_modes::semantic, node_id, env, "subcommand failed",
					wpp::cat("subprocess exited with non-zero status `", cmd, "`")
				);

			return str;
		#endif
	}


	std::string intrinsic_pipe(
		wpp::node_t node_id,
		wpp::node_t cmd_id,
		wpp::node_t value_id,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		DBG();

		#if defined(WPP_DISABLE_RUN)
			wpp::error(report_modes::semantic, node_id, env, "intrinsic disabled", "`pipe` not available");

		#else
			if (env.flags & wpp::FLAG_DISABLE_RUN)
				wpp::error(report_modes::semantic, node_id, env, "intrinsic disabled", "`pipe` not available");

			std::string str;

			const auto cmd = evaluate(cmd_id, env, fn_env);
			const auto data = evaluate(value_id, env, fn_env);

			int rc = 0;
			std::string out = wpp::exec(cmd, data, rc);

			// trim trailing newline.
			if (out.back() == '\n')
				out.erase(out.end() - 1, out.end());

			if (rc)
				wpp::error(report_modes::semantic, node_id, env, "subcommand failed",
					wpp::cat("subprocess exited with non-zero status `", cmd, "`")
				);

			return out;
		#endif
	}


	std::string intrinsic_file(
		wpp::node_t node_id,
		wpp::node_t expr,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		DBG();

		#if defined(WPP_DISABLE_FILE)
			wpp::error(report_modes::semantic, node_id, env, "instrinsic disabled", "`file` not available");

		#else
			if (env.flags & wpp::FLAG_DISABLE_FILE)
				wpp::error(report_modes::semantic, node_id, env, "intrinsic disabled", "`file` not available");

			const auto fname = wpp::evaluate(expr, env, fn_env);

			if (fname.empty())
				wpp::error(report_modes::semantic, node_id, env, "empty path", "`file` must be supplied a non-empty string");

			try {
				return wpp::read_file(std::filesystem::relative(std::filesystem::path{fname}));
			}

			catch (const wpp::FileError&) {
				wpp::error(report_modes::semantic, node_id, env, "could not read file", wpp::cat("file '", fname, "' does not exist or could not be found"));
			}

			return "";
		#endif
	}


	std::string intrinsic_use(
		wpp::node_t node_id,
		wpp::node_t expr,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		DBG();

		#if defined(WPP_DISABLE_FILE)
			wpp::error(report_modes::semantic, node_id, env, "instrinsic disabled", "`use` not available");

		#else
			if (env.flags & wpp::FLAG_DISABLE_FILE)
				wpp::error(report_modes::semantic, node_id, env, "intrinsic disabled", "`use` not available");


			std::string str;
			const auto fname = wpp::evaluate(expr, env, fn_env);

			if (fname.empty())
				wpp::error(report_modes::semantic, node_id, env, "empty path", "`use` must be supplied a non-empty string");

			std::filesystem::path old_path, new_path;
			std::string source;

			try {
				// Store current path and get the path of the new file.
				old_path = std::filesystem::current_path();
				new_path = old_path / wpp::get_file_path(fname, env.path);

				// Don't source something we've already seen.
				if (env.sources.is_previously_seen(new_path))
					return "";

				std::filesystem::current_path(new_path.parent_path());
			}

			catch (const wpp::FileError&) {
				wpp::error(report_modes::semantic, node_id, env, "could not find file", wpp::cat("file '", fname, "' does not exist or could not be found"));
			}

			try {
				source = wpp::read_file(old_path / new_path);
			}

			catch (const wpp::FileError&) {
				wpp::error(report_modes::semantic, node_id, env, "could not read file", wpp::cat("there was an error while reading file '", fname, "'"));
			}

			env.sources.push(new_path, source, wpp::modes::source);
			str = wpp::evaluate(wpp::parse(env), env, fn_env);

			std::filesystem::current_path(old_path);

			return str;
		#endif
	}


	std::string intrinsic_assert(
		wpp::node_t node_id,
		wpp::node_t lhs,
		wpp::node_t rhs,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		DBG();

		// Check if strings are equal.
		const auto str_a = evaluate(lhs, env, fn_env);
		const auto str_b = evaluate(rhs, env, fn_env);

		if (str_a != str_b)
			wpp::error(report_modes::semantic, node_id, env, "assertion failed", wpp::cat("lhs='", str_a, "', rhs='", str_b, "'"));

		return "";
	}


	std::string intrinsic_error(
		wpp::node_t node_id,
		wpp::node_t expr,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		DBG();

		const auto msg = evaluate(expr, env, fn_env);
		wpp::error(report_modes::semantic, node_id, env, "user error", msg);

		return "";
	}


	std::string intrinsic_log(
		wpp::node_t node_id,
		wpp::node_t expr,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		DBG();
		std::cerr << evaluate(expr, env, fn_env);
		return "";
	}
}

