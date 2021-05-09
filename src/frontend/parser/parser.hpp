#pragma once

#ifndef WOTPP_PARSER
#define WOTPP_PARSER

#include <misc/fwddecl.hpp>
#include <structures/environment.hpp>
#include <frontend/lexer/lexer.hpp>

// The meat of wot++, the parser.
// This is a plain old LL(1) predictive recursive descent parser.

namespace wpp {
	wpp::node_t document(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);

	inline wpp::node_t parse(wpp::Env& env) {
		wpp::Lexer lex{ env };
		return wpp::document(0, lex, env.ast, env.ast_meta, env);
	}
}

#endif
