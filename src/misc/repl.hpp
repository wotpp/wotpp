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

#include <frontend/lexer/lexer.hpp>
#include <frontend/parser/parser.hpp>

#include <backend/eval/eval.hpp>

namespace wpp {
	inline int repl() {
		// #ifdef WPP_DISABLE_REPL
		// 	std::cerr << "REPL support is disabled\n";
		// 	return 1;

		// #else
		// 	const auto path = std::filesystem::current_path();

		// 	wpp::Context ctx{ path, path, "repl", nullptr };
		// 	wpp::Env env{ ctx, wpp::warning_t{} };

		// 	std::cout << "wot++ repl\n";

		// 	using_history();

		// 	while (true) {
		// 		auto input = readline(">>> ");

		// 		if (!input) // EOF
		// 			break;

		// 		add_history(input);

		// 		wpp::Lexer lex{ ctx };

		// 		std::string out = wpp::evaluate(wpp::parse(lex, env), env);
		// 		std::cout << out << std::flush;

		// 		if (out.size() && out[out.size() - 1] != '\n')
		// 			std::cout << std::endl;

		// 		std::free(input);
		// 	}

		// 	return 0;
		// #endif

		return 1;
	}
}

#endif
