#pragma once

#ifndef WOTPP_REPL
#define WOTPP_REPL

#include <iosfwd>

#ifndef WPP_DISABLE_REPL
	extern "C" {
		#include <linenoise/linenoise.h>
	}

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

			linenoiseSetMultiLine(true);
			linenoiseHistorySetMaxLen(50);

			const auto initial_path = std::filesystem::current_path();
			wpp::Env env{ initial_path, {}, wpp::flags_t{wpp::WARN_ALL} };


			char* input = nullptr;

			while ((input = linenoise(">>> ")) != nullptr) {
				// Reset error state.
				env.state &= ~wpp::INTERNAL_ERROR_STATE;

				linenoiseHistoryAdd(input);

				env.sources.push(initial_path, input, modes::repl);

				try {
					wpp::node_t root = wpp::parse(env);

					if (env.state & wpp::INTERNAL_ERROR_STATE)
						return 1;

					std::string out = wpp::evaluate(root, env);

					if (not out.empty() and out.back() != '\n')
						out += '\n';

					std::cout << out << std::flush;

				} catch (const wpp::Report& e) {
					std::cerr << e.str();
				}

				std::free(input);
			}

			return 0;
		#endif
	}
}

#endif
