#include <string>
#include <iostream>
#include <utility>
#include <chrono>

#include <cstdint>
#include <cstring>
#include <ctime>

#include <misc/warnings.hpp>
#include <backend/eval/eval.hpp>
#include <misc/repl.hpp>
#include <misc/argp.hpp>


constexpr auto ver = "alpha-git";
constexpr auto desc = "A small macro language for producing and manipulating strings.";


int main(int argc, const char* argv[]) {
	std::string_view inputf, outputf;
	std::vector<std::string_view> warnings;
	bool repl = false, sexpr = false;


	std::vector<const char*> positional;

	if (wpp::argparser(
		wpp::Meta{ver, desc},
		argc, argv, positional,
		wpp::Opt{inputf,   "input file",  	   "--input",    "-i"},
		wpp::Opt{outputf,  "output file", 	   "--output",   "-o"},
		wpp::Opt{repl,     "repl mode",   	   "--repl",     "-R"},
		wpp::Opt{sexpr,    "print the AST",     "--sexpr",    "-S"},
		wpp::Opt{warnings, "toggle warnings",   "--warnings", "-W"}
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

		else {
			std::cerr << "unrecognized warning: '" << x << "'.\n";
			return 1;
		}
	}


	if (repl)
		return wpp::repl();

	else if (sexpr)
		return 1;

	else if (not inputf.empty())
		return wpp::run(std::string{inputf}, warning_flags);


	return 1;
}
