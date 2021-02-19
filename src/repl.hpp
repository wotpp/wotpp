#pragma once

#ifndef WOTPP_REPL
#define WOTPP_REPL

#ifndef WPP_DISABLE_REPL
	#include <readline/readline.h>
	#include <readline/history.h>
	#include <cstdlib>
#endif

#include <lexer.hpp>
#include <parser.hpp>
#include <genprog.hpp>
#include <exception.hpp>
#include <visitors/sexpr.hpp>
#include <visitors/eval.hpp>

#include <tinge.hpp>

namespace wpp {
	int repl() {
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
					const auto out = wpp::eval_ast(root, env);
					std::cout << out << std::flush;

					if (out.size() && out[out.size() - 1] != '\n')
						std::cout << std::endl;
				}

				catch (const wpp::Exception& e) {
					wpp::error(e.pos, e.what());
				}

				std::free(input);
			}

			return 0;
		#endif
	}
}

#endif
