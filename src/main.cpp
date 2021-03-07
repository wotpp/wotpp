#include <string_view>
#include <filesystem>
#include <string>
#include <vector>
#include <iostream>
#include <utility>
#include <chrono>

#include <misc/warnings.hpp>
#include <misc/util/util.hpp>
#include <misc/repl.hpp>
#include <misc/argp.hpp>
#include <backend/eval/eval.hpp>


constexpr auto ver = "alpha-git";
constexpr auto desc = "A small macro language for producing and manipulating strings.";


int main(int argc, const char* argv[]) {
	auto t1 = std::chrono::steady_clock::now();

	std::string_view outputf;
	std::vector<std::string_view> warnings;
	bool repl = false, enable_run = true, force = false;


	std::vector<const char*> positional;

	if (wpp::argparser(
		wpp::Meta{ver, desc},
		argc, argv, &positional,
		wpp::Opt{outputf,    "output file",                 "--output",   "-o"},
		wpp::Opt{warnings,   "toggle warnings",             "--warnings", "-W"},
		wpp::Opt{repl,       "repl mode",                   "--repl",     "-r"},
		wpp::Opt{enable_run, "toggle run intrinsic",        "--run",      "-R"},
		wpp::Opt{force,      "overwrite file if it exists", "--force",    "-f"}
	))
		return 0;


	wpp::warning_t warning_flags = 0;

	for (const auto& x: warnings) {
		if (x == "param-shadow-func")
			warning_flags |= wpp::WARN_PARAM_SHADOW_FUNC;

		else if (x == "param-shadow-param")
			warning_flags |= wpp::WARN_PARAM_SHADOW_PARAM;

		else if (x == "func-redefined")
			warning_flags |= wpp::WARN_FUNC_REDEFINED;

		else if (x == "varfunc-redefined")
			warning_flags |= wpp::WARN_VARFUNC_REDEFINED;

		else if (x == "all")
			warning_flags = wpp::WARN_ALL;

		else {
			std::cerr << "unrecognized warning: '" << x << "'.\n";
			return 1;
		}
	}


	if (repl)
		return wpp::repl();


	if (positional.empty()) {
		std::cerr << "no input files.\n";
		return 1;
	}


	std::string out;
	const auto initial_path = std::filesystem::current_path();

	for (const auto& fname: positional) {
		try {
			// Set current path to path of file.
			const auto path = initial_path / std::filesystem::path{fname};
			std::filesystem::current_path(path.parent_path());

			wpp::Env env{ initial_path, warning_flags };
			env.sources.push(path, wpp::read_file(path), wpp::modes::normal);

			try {
				out += wpp::evaluate(wpp::parse(env), env);
			}

			catch (wpp::Error& e) {
				return 1;
			}
		}

		catch (const std::filesystem::filesystem_error&) {
			std::cerr << "file '" << fname << "' not found.\n";
			return 1;
		}

		std::filesystem::current_path(initial_path);
	}

	if (not outputf.empty()) {
		std::error_code ec;
		if (not force and std::filesystem::exists(outputf, ec)) {
			std::cerr << "file '" << outputf << "' exists.\n";
			return 1;
		}

		wpp::write_file(outputf, out);
	}

	else
		std::cout << out;

	auto t2 = std::chrono::steady_clock::now();

	std::chrono::duration<double> s = t2 - t1;
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(s);
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(s);

	std::cerr << "time: " <<
		s.count() << "s  " <<
		ms.count() << "ms  " <<
		us.count() << "us\n"
	;

	return 0;
}
