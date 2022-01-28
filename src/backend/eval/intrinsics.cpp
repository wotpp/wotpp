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

		try {
			str = wpp::evaluate(wpp::parse(env, node_id), env, fn_env);
		}

		catch (const wpp::Report& e) {
			env.state |=
				wpp::ABORT_EVALUATION |
				wpp::ERROR_MODE_EVAL;

			wpp::error(e.report_mode, node_id, env, e.overview, e.detail, e.suggestion);
		}

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
				try {
					return wpp::read_file(std::filesystem::relative(std::filesystem::path{fname}));
				}

				catch (const std::filesystem::filesystem_error&) {
					throw wpp::FileReadError{};
				}
			}

			catch (const wpp::FileNotFoundError&) {
				wpp::error(report_modes::semantic, node_id, env, "could not find file",
					wpp::cat("file '", fname, "' could not be found")
				);
			}

			catch (const wpp::NotFileError&) {
				wpp::error(report_modes::semantic, node_id, env, "not a file",
					wpp::cat("'", fname, "' is not file")
				);
			}

			catch (const wpp::FileReadError&) {
				wpp::error(report_modes::semantic, node_id, env, "could not read file",
					wpp::cat("file '", fname, "' could not be read")
				);
			}

			catch (const wpp::SymlinkError&) {
				wpp::error(report_modes::semantic, node_id, env, "invalid symlink",
					wpp::cat("symlink '", fname, "' resolves to itself")
				);
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
				try {
					// Store current path and get the path of the new file.
					old_path = std::filesystem::current_path();
					new_path = old_path / wpp::get_file_path(fname, env.path);

					// Don't source something we've already seen.
					if (env.sources.is_previously_seen(new_path))
						return "";

					std::filesystem::current_path(new_path.parent_path());

					source = wpp::read_file(old_path / new_path);
				}

				catch (const std::filesystem::filesystem_error&) {
					throw wpp::FileReadError{};
				}
			}

			catch (const wpp::FileNotFoundError&) {
				wpp::error(report_modes::semantic, node_id, env, "could not find file",
					wpp::cat("file '", fname, "' could not be found")
				);
			}

			catch (const wpp::NotFileError&) {
				wpp::error(report_modes::semantic, node_id, env, "not a file",
					wpp::cat("'", fname, "' is not file")
				);
			}

			catch (const wpp::FileReadError&) {
				wpp::error(report_modes::semantic, node_id, env, "could not read file",
					wpp::cat("file '", fname, "' could not be read")
				);
			}

			catch (const wpp::SymlinkError&) {
				wpp::error(report_modes::semantic, node_id, env, "invalid symlink",
					wpp::cat("symlink '", fname, "' resolves to itself")
				);
			}

			try {
				env.sources.push(new_path, source, wpp::modes::source);
				str = wpp::evaluate(wpp::parse(env, node_id), env, fn_env);
			}

			catch (const wpp::Report& e) {
				env.state |=
					wpp::ABORT_EVALUATION |
					wpp::ERROR_MODE_EVAL;
				;

				throw;
			}

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

