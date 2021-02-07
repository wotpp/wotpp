#include <string>
#include <iostream>
#include <utility>
#include <chrono>

#include <cstdint>
#include <cstring>
#include <ctime>

#include <lexer.hpp>
#include <parser.hpp>
#include <genprog.hpp>
#include <visitors/sexpr.hpp>
#include <visitors/eval.hpp>

#include <tinge.hpp>

int main(int argc, const char* argv[]) {
	if (argc != 2) {
		tinge::errorln("usage: wpp <file>");
		return 1;
	}

	auto file = wpp::read_file(argv[1]);

	wpp::Lexer lex{file.c_str()};
	wpp::AST tree;

	// Reserve 10MiB
	tree.reserve((1024 * 1024 * 10) / sizeof(decltype(tree)::value_type));

	// Parse.
	auto root = document(lex, tree);

	// Print S-Expression.
	// std::cout << wpp::print_ast(root, tree) << '\n';

	// Evaluate.
	wpp::Environment env;
	std::cout << wpp::eval_ast(root, tree, env);

	return 0;
}
