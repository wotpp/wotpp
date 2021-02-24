#pragma once

#ifndef WOTPP_REPL
#define WOTPP_REPL

#include <iostream>

#ifndef WPP_DISABLE_REPL
	#include <readline/readline.h>
	#include <readline/history.h>
	#include <cstdlib>
#endif

#include <misc/util/util.hpp>
#include <structures/exception.hpp>

#include <frontend/lexer/lexer.hpp>
#include <frontend/parser/parser.hpp>

#include <backend/sexpr/sexpr.hpp>
#include <backend/eval/eval.hpp>

namespace wpp {
	inline int repl() {
		#ifdef WPP_DISABLE_REPL
			std::cerr << "REPL support is disabled\n";
			return 1;

		#else
			wpp::AST tree;
			wpp::Environment env{std::filesystem::current_path(), tree};

			// Reserve 10MiB
			tree.reserve((1024 * 1024 * 10) / sizeof(decltype(tree)::value_type));

			std::cout << "wot++ repl\n";

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
