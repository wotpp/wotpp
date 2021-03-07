#pragma once

#ifndef WOTPP_REPL
#define WOTPP_REPL

#include <iosfwd>

#ifndef WPP_DISABLE_REPL
	#include <readline/readline.h>
	#include <readline/history.h>
	#include <cstdlib>

	#include <misc/util/util.hpp>
	#include <frontend/parser/parser.hpp>
	#include <backend/eval/eval.hpp>
#endif


namespace wpp {
	inline int repl() {
		#ifdef WPP_DISABLE_REPL
			std::cerr << "REPL support is disabled\n";
			return 1;

		#else
			std::cout << "wot++ repl\n";

			const auto path = std::filesystem::current_path();
			wpp::Env env{ path, wpp::flags_t{} };

			using_history();

			while (true) {
				char* input = readline(">>> ");

				if (not input) // EOF
					break;

				add_history(input);

				env.sources.push(path, input, modes::repl);

				try {
					std::string out = wpp::evaluate(wpp::parse(env), env);

					if (not out.empty() and out.back() != '\n')
						out += '\n';

					std::cout << out << std::flush;

				} catch (const wpp::Error&) {}

				env.sources.pop();
				std::free(input);
			}

			return 0;
		#endif
	}
}

#endif
