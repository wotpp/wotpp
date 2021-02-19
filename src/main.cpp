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
#include <lexer.hpp>
#include <parser.hpp>
#include <genprog.hpp>
#include <exception.hpp>
#include <misc/argp.hpp>

int main(int argc, const char* argv[]) {
	wpp::warning_t warning_flags =
		wpp::WARN_FUNC_REDEFINED |
		wpp::WARN_PARAM_SHADOW_FUNC |
		wpp::WARN_PARAM_SHADOW_PARAM |
		wpp::WARN_VARFUNC_REDEFINED
	;
	wpp::ArgResult test;

	wpp::ArgResult input, output, sexpr;
	
	auto usage = "w++ -i INPUT [-o OUTPUT] [-sh]";

	auto argparser = wpp::ArgumentParser("wpp", "A small macro language for producing and manipulating strings", "alpha-git", usage)
		.arg(&input, "File to read input from", "input", "i", true)
		.arg(&output, "File to output to (stdout by default)", "output", "o", true)
		.arg(&sexpr, "Print AST as S-expression", "sexpr", "s", false);

	argparser.parse(argc, argv);
	
	if (argc == 1)
		return wpp::repl();

	else if (argc == 2)
		return wpp::run(argv[1], warning_flags);

	else
		std::cerr << "usage: wpp [file]\n";

	return 1;
}
