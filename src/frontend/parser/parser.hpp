#pragma once

#ifndef WOTPP_PARSER
#define WOTPP_PARSER

#include <misc/fwddecl.hpp>
#include <structures/environment.hpp>
#include <frontend/lexer/lexer.hpp>

// The meat of wot++, the parser.
// This is a plain old LL(1) predictive recursive descent parser.

namespace wpp {
	wpp::node_t document(wpp::Lexer&, wpp::AST&, wpp::Positions&, wpp::Env&);

	inline wpp::node_t parse(wpp::Env& env) {
		wpp::Lexer lex{ env };
		return wpp::document(lex, env.ast, env.positions, env);
	}
}

#endif
