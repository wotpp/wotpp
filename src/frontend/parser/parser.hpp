#pragma once

#ifndef WOTPP_PARSER
#define WOTPP_PARSER

#include <string>

#include <frontend/token.hpp>
#include <frontend/lexer/lexer.hpp>
#include <frontend/parser/ast_nodes.hpp>

// The meat of wot++, the parser.
// This is a plain old LL(1) predictive recursive descent parser.

namespace wpp {
	inline bool peek_is_intrinsic(const wpp::Token& tok) {
		return
			tok == TOKEN_RUN or
			tok == TOKEN_EVAL or
			tok == TOKEN_FILE or
			tok == TOKEN_ASSERT or
			tok == TOKEN_PIPE or
			tok == TOKEN_ERROR or
			tok == TOKEN_SOURCE or
			tok == TOKEN_SLICE or
			tok == TOKEN_FIND or
			tok == TOKEN_LENGTH or
			tok == TOKEN_ESCAPE or
			tok == TOKEN_LOG
		;
	}

	inline bool peek_is_keyword(const wpp::Token& tok) {
		return
			tok == TOKEN_LET or
			tok == TOKEN_VAR or
			tok == TOKEN_PREFIX or
			tok == TOKEN_DROP
		;
	}

	// Check if the token is a string.
	inline bool peek_is_string(const wpp::Token& tok) {
		return
			tok == TOKEN_DOUBLEQUOTE or
			tok == TOKEN_QUOTE or

			tok == TOKEN_EXCLAIM or

			tok == TOKEN_HEX or
			tok == TOKEN_BIN or

			tok == TOKEN_SMART
		;
	}

	inline bool peek_is_reserved_name(const wpp::Token& tok) {
		return
			peek_is_intrinsic(tok) or
			peek_is_keyword(tok)
		;
	}

	// Check if the token is an expression.
	inline bool peek_is_call(const wpp::Token& tok) {
		return
			tok == TOKEN_IDENTIFIER or
			peek_is_intrinsic(tok)
		;
	}

	// Check if the token is an expression.
	inline bool peek_is_expr(const wpp::Token& tok) {
		return
			tok == TOKEN_MAP or
			tok == TOKEN_EQUAL or
			tok == TOKEN_LBRACE or
			peek_is_string(tok) or
			peek_is_call(tok)
		;
	}

	// Check if the token is a statement.
	inline bool peek_is_stmt(const wpp::Token& tok) {
		return
			peek_is_keyword(tok) or
			peek_is_expr(tok)
		;
	}
}


namespace wpp {
	// Consume tokens comprising a string. Handles escape chars.
	void accumulate_string(const wpp::Token&, std::string&, bool = true);

	// Forward declarations.
	void normal_string(wpp::Lexer&, std::string&);
	void stringify_string(wpp::Lexer&, std::string&);
	void smart_string(wpp::Lexer&, std::string&);
	void hex_string(wpp::Lexer&, std::string&);
	void bin_string(wpp::Lexer&, std::string&);

	void raw_string(std::string&);
	void para_string(std::string&);
	void code_string(std::string&);

	wpp::node_t expression(wpp::Lexer&, wpp::AST&);
	wpp::node_t fninvoke(wpp::Lexer&, wpp::AST&);
	wpp::node_t map(wpp::Lexer&, wpp::AST&);
	wpp::node_t block(wpp::Lexer&, wpp::AST&);
	wpp::node_t codeify(wpp::Lexer&, wpp::AST&);
	wpp::node_t string(wpp::Lexer&, wpp::AST&);

	wpp::node_t statement(wpp::Lexer&, wpp::AST&);
	wpp::node_t var(wpp::Lexer&, wpp::AST&);
	wpp::node_t drop(wpp::Lexer&, wpp::AST&);
	wpp::node_t let(wpp::Lexer&, wpp::AST&);
	wpp::node_t prefix(wpp::Lexer&, wpp::AST&);

	wpp::node_t document(wpp::Lexer&, wpp::AST&);
}

#endif
