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

int main(int argc, const char* argv[]) {
	wpp::ArgResult input, output, sexpr, repl, lint;

	auto usage = "w++ -i INPUT [-o OUTPUT] [-srh]";

	auto argparser = wpp::ArgumentParser(
		"wot++",
		"A small macro language for producing and manipulating strings",
		"alpha-git",
		usage
	)
		.arg(&input,  "File to read input from",                        "input",  "i", true)
		.arg(&output, "File to output to (stdout by default)",          "output", "o", true)
		.arg(&sexpr,  "Print AST as S-expression",                      "sexpr",  "s", false)
		.arg(&repl,   "Start an interactive prompt",                    "repl",   "r", false)
		.arg(&lint,   "Enable various warnings (comma-seperated list)", "lint",   "l", true);

	wpp::warning_t warning_flags = 0;

	if (not argparser.parse(argc, argv))
		return 1;

	// Enable any lints
	if (lint.is_present) {
		std::istringstream list(lint.value);
		std::string buf;

		while (std::getline(list, buf, ',')) {
			if      (buf == "param-shadow-func")
				warning_flags |= wpp::WARN_PARAM_SHADOW_FUNC;
			else if (buf == "param-shadow-param")
				warning_flags |= wpp::WARN_PARAM_SHADOW_PARAM;
			else if (buf == "func-redefined")
				warning_flags |= wpp::WARN_FUNC_REDEFINED;
			else if (buf == "varfunc-redefined")
				warning_flags |= wpp::WARN_VARFUNC_REDEFINED;
			else {
				std::cout << "w++: unrecognized lint: " << buf << std::endl;
				return 1;
			}
		}
	}

	if (repl.is_present)
		return wpp::repl();

	else if (input.is_present)
		return wpp::run(input.value, warning_flags);

	return 1;
}
