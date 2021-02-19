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
#include <exception.hpp>
#include <visitors/sexpr.hpp>
#include <visitors/eval.hpp>
#include <utils/argp.hpp>

#include <tinge.hpp>

#ifndef WPP_DISABLE_REPL
	#include <readline/readline.h>
	#include <readline/history.h>
	#include <cstdlib>
#endif


int main(int argc, const char* argv[]) {
	wpp::ArgResult input, output, sexpr, repl;
	
	auto usage = "w++ -i INPUT [-o OUTPUT] [-sh]";

	auto argparser = wpp::ArgumentParser(
		"wpp", "A small macro language for producing and manipulating strings", 
		"alpha-git", usage)
		.arg(&input,  "File to read input from",               "input",  "i", true)
//		.arg(&output, "File to output to (stdout by default)", "output", "o", true)
//		.arg(&sexpr,  "Print AST as S-expression",             "sexpr",  "s", false)
		.arg(&repl,   "Start an interactive prompt",           "repl",   "r", false);

	if (!argparser.parse(argc, argv)) 
		return -1;

	if (repl.is_present) {
		#ifdef WPP_DISABLE_REPL
			tinge::errorln("REPL support is disabled");
			return 1;
		#else
			wpp::AST tree;
			wpp::Environment env{tree};

			// Reserve 10MiB
			tree.reserve((1024 * 1024 * 10) / sizeof(decltype(tree)::value_type));

			tinge::println("wot++ repl");

			using_history();

			while (true) {
				auto input = readline(">>> ");
				if (!input) // EOF
					break;

				add_history(input);

				// Create a new lexer.
				wpp::Lexer lex{"<repl>", input};

				try {
					// Parse.
					auto root = document(lex, tree);

        				// Evaluate.
					auto out = wpp::eval_ast(root, env);
					std::cout << out << std::flush;

					if (out.size() && out[out.size() - 1] != '\n')
						std::cout << std::endl;
				}

				catch (const wpp::Exception& e) {
					wpp::error(e.pos, e.what());
				}

				std::free(input);
			}
		#endif

	} else if (input.is_present) {
		std::string file;

		try {
			file = wpp::read_file(input.value);
		}

		catch (const std::filesystem::filesystem_error& e) {
			tinge::errorln("file not found.");
			return 1;
		}

		// Set current path to path of file.
		std::filesystem::current_path(std::filesystem::current_path() / std::filesystem::path{argv[1]}.parent_path());

		try {
			wpp::Lexer lex{argv[1], file.c_str()};
			wpp::AST tree;
			wpp::Environment env{tree};

			tree.reserve((1024 * 1024 * 10) / sizeof(decltype(tree)::value_type));

			auto root = wpp::document(lex, tree);
			std::cout << wpp::eval_ast(root, env) << std::flush;
		}

		catch (const wpp::Exception& e) {
			wpp::error(e.pos, e.what());
			return 1;
		}

	}

	return 0;
}
