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

#include <tinge.hpp>

#ifndef WPP_DISABLE_REPL
	#include <readline/readline.h>
	#include <readline/history.h>
	#include <cstdlib>
#endif

int main(int argc, const char* argv[]) {
	// Create new syntax tree, environment and root caller
	wpp::AST tree;
	wpp::Environment env{tree};
	wpp::Caller root_caller;

	// Reserve 10MiB
	tree.reserve((1024 * 1024 * 10) / sizeof(decltype(tree)::value_type));

	if (argc == 1) {
		#ifdef WPP_DISABLE_REPL
			tinge::errorln("REPL support is disabled");
			return 1;

		#else
			tinge::println("wot++ repl");

			using_history();

			while (true) {
				auto input = readline(">>> ");
				if (!input) // EOF
					break;

				add_history(input);

				// Create a new lexer.
				wpp::Lexer lex{input};

				try {
					// Parse.
					auto root = document(lex, tree);

					// Evaluate.
					auto out = wpp::eval_ast(root, env, root_caller);
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

	} else if (argc == 2) {
		std::string file;

		try {
			file = wpp::read_file(argv[1]);
		}

		catch (const std::runtime_error& e) {
			tinge::errorln("file not found.");
		}

		// Set current path to path of file.
		std::filesystem::current_path(std::filesystem::current_path() / std::filesystem::path{argv[1]}.parent_path());

		try {
			// Create a new lexer
			wpp::Lexer lex{file.c_str()};

			// Reserve 10MiB
			tree.reserve((1024 * 1024 * 10) / sizeof(decltype(tree)::value_type));

			// Parse.
			auto root = document(lex, tree);

			// Evaluate.
			std::cout << wpp::eval_ast(root, env, root_caller) << std::flush;
		}

		catch (const wpp::Exception& e) {
			wpp::error(e.pos, e.what());
			return 1;
		}

	} else {
		tinge::errorln("usage: wpp [file]");
		return 1;
	}

	return 0;
}
