#include <string_view>
#include <iomanip>
#include <filesystem>
#include <string>
#include <vector>
#include <iostream>
#include <utility>
#include <chrono>

#include <misc/flags.hpp>
#include <misc/util/util.hpp>
#include <misc/repl.hpp>
#include <misc/argp.hpp>
#include <backend/eval/eval.hpp>
#include <frontend/parser/parser.hpp>


constexpr auto ver = "alpha-git";
constexpr auto desc = "A small macro language for producing and manipulating strings.";


int main(int argc, const char* argv[]) {
	auto t1 = std::chrono::steady_clock::now();

	std::string_view outputf;
	std::vector<std::string_view> warnings;
	bool repl = false, disable_run = false, force = false;


	std::vector<const char*> positional;

	if (wpp::argparser(
		wpp::Meta{ver, desc},
		argc, argv, &positional,
		wpp::Opt{outputf,     "output file",                 "--output",      "-o"},
		wpp::Opt{warnings,    "toggle warnings",             "--warnings",    "-W"},
		wpp::Opt{repl,        "repl mode",                   "--repl",        "-r"},
		wpp::Opt{disable_run, "disable run intrinsic",       "--disable-run", "-R"},
		wpp::Opt{force,       "overwrite file if it exists", "--force",       "-f"}
	))
		return 0;


	wpp::flags_t flags = 0;

	for (const auto& x: warnings) {
		if (x == "param-shadow-func")
			flags |= wpp::WARN_PARAM_SHADOW_FUNC;

		else if (x == "param-shadow-param")
			flags |= wpp::WARN_PARAM_SHADOW_PARAM;

		else if (x == "func-redefined")
			flags |= wpp::WARN_FUNC_REDEFINED;

		else if (x == "varfunc-redefined")
			flags |= wpp::WARN_VARFUNC_REDEFINED;

		else if (x == "all")
			flags = wpp::WARN_ALL;

		else {
			std::cerr << "unrecognized warning: '" << x << "'.\n";
			return 1;
		}
	}


	if (disable_run)
		flags |= wpp::FLAG_DISABLE_RUN;


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

			wpp::Env env{ initial_path, flags };
			env.sources.push(path, wpp::read_file(path), wpp::modes::normal);
			out += wpp::evaluate(wpp::parse(env), env);
		}

		catch (wpp::Error& e) {
			return 1;
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

	std::cerr << "took: " << std::fixed << std::setprecision(4) << std::chrono::duration<double>(t2 - t1).count() << "s\n";

	return 0;
}
