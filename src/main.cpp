#include <string>
#include <iostream>
#include <utility>
#include <chrono>

#include <cstdint>
#include <cstring>
#include <ctime>

#include <visitors/eval.hpp>
#include <utils/argp.hpp>
#include <repl.hpp>

#include <tinge.hpp>

int main(int argc, const char* argv[]) {
	wpp::ArgResult input, output, sexpr, repl;
	
	auto usage = "w++ -i INPUT [-o OUTPUT] [-sh]";

	auto argparser = wpp::ArgumentParser(
		"wot++", 
		"A small macro language for producing and manipulating strings", 
		"alpha-git", 
		usage
	)
		.arg(&input,  "File to read input from",               "input",  "i", true)
//		.arg(&output, "File to output to (stdout by default)", "output", "o", true)
//		.arg(&sexpr,  "Print AST as S-expression",             "sexpr",  "s", false)
		.arg(&repl,   "Start an interactive prompt",           "repl",   "r", false);

	if (!argparser.parse(argc, argv)) 
		return -1;

	if (repl.is_present)
		return wpp::repl();

	else if (input.is_present)
		return wpp::run(input.value);

	return 1;
}
