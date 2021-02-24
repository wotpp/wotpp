#pragma once

#ifndef WOTPP_LEXER
#define WOTPP_LEXER

#include <string>
#include <utility>

#include <frontend/token.hpp>
#include <frontend/position.hpp>

// Token types.
namespace wpp {
	// A macro to define our token types.
	// We use a macro of nested macros here so that
	// we can simultaneously create both an enum
	// of token types and a const char* array
	// of names.
	// i.e. TOKEN(identifier) =>
	//   tokens::identifier
	//   tokens::to_str[tokens::identifier]

	#define TOKEN_TYPES \
		TOKEN(TOKEN_NONE) \
		TOKEN(TOKEN_EOF) \
		TOKEN(TOKEN_CHAR) \
		\
		TOKEN(TOKEN_WHITESPACE) \
		TOKEN(TOKEN_SLASH) \
		TOKEN(TOKEN_BACKSLASH) \
		TOKEN(TOKEN_CAT) \
		TOKEN(TOKEN_ARROW) \
		TOKEN(TOKEN_COMMA) \
		TOKEN(TOKEN_STAR) \
		TOKEN(TOKEN_BAR) \
		TOKEN(TOKEN_EQUAL) \
		TOKEN(TOKEN_EXCLAIM) \
		\
		TOKEN(TOKEN_IDENTIFIER) \
		TOKEN(TOKEN_MAP) \
		TOKEN(TOKEN_PREFIX) \
		TOKEN(TOKEN_LET) \
		TOKEN(TOKEN_DROP) \
		TOKEN(TOKEN_VAR) \
		\
		TOKEN(TOKEN_RUN) \
		TOKEN(TOKEN_FILE) \
		TOKEN(TOKEN_EVAL) \
		TOKEN(TOKEN_LENGTH) \
		TOKEN(TOKEN_SLICE) \
		TOKEN(TOKEN_FIND) \
		TOKEN(TOKEN_ASSERT) \
		TOKEN(TOKEN_ERROR) \
		TOKEN(TOKEN_PIPE) \
		TOKEN(TOKEN_SOURCE) \
		TOKEN(TOKEN_ESCAPE) \
		TOKEN(TOKEN_LOG) \
		\
		TOKEN(TOKEN_LPAREN) \
		TOKEN(TOKEN_RPAREN) \
		TOKEN(TOKEN_LBRACE) \
		TOKEN(TOKEN_RBRACE) \
		\
		TOKEN(TOKEN_ESCAPE_NEWLINE) \
		TOKEN(TOKEN_ESCAPE_TAB) \
		TOKEN(TOKEN_ESCAPE_CARRIAGERETURN) \
		TOKEN(TOKEN_ESCAPE_QUOTE) \
		TOKEN(TOKEN_ESCAPE_DOUBLEQUOTE) \
		TOKEN(TOKEN_ESCAPE_BACKSLASH) \
		TOKEN(TOKEN_ESCAPE_HEX) \
		TOKEN(TOKEN_ESCAPE_BIN) \
		\
		TOKEN(TOKEN_STRING) \
		TOKEN(TOKEN_DOUBLEQUOTE) \
		TOKEN(TOKEN_QUOTE) \
		TOKEN(TOKEN_HEX) \
		TOKEN(TOKEN_BIN) \
		TOKEN(TOKEN_SMART) \
		\
		TOKEN(TOKEN_TOTAL)

	#define TOKEN(x) x,
		enum: token_type_t { TOKEN_TYPES };
	#undef TOKEN

	#define TOKEN(x) #x,
		constexpr const char* to_str[] = { TOKEN_TYPES };
	#undef TOKEN

	#undef TOKEN_TYPES
}

namespace wpp {
	class Lexer;

	void lex_literal(wpp::token_type_t, bool(*)(char), wpp::Lexer&, wpp::Token&);
	void lex_simple(wpp::token_type_t, int, wpp::Lexer&, wpp::Token&);

	void lex_comment(wpp::Lexer&, wpp::Token&);
	void lex_single_comment(wpp::Lexer&, wpp::Token&);
	void lex_whitespace(wpp::Lexer&, wpp::Token&);

	void lex_identifier(wpp::Lexer&, wpp::Token&);

	void lex_smart(wpp::Lexer&, wpp::Token&);
	void lex_string_escape(wpp::Lexer&, wpp::Token&);
	void lex_string_other(wpp::Lexer&, wpp::Token&);

	void lex_mode_string(wpp::Lexer&, wpp::Token&);
	void lex_mode_normal(wpp::Lexer&, wpp::Token&);
}

namespace wpp::modes {
	enum {
		normal,
		string,
		character,
	};
}

namespace wpp {
	struct Lexer {
		std::string fname;
		const char* const start = nullptr;
		const char* str = nullptr;

		wpp::Token lookahead{};
		int lookahead_mode = modes::normal;


		Lexer(const std::string& fname_, const char* const str_, int mode_ = modes::normal):
			fname(fname_),
			start(str_),
			str(str_),
			lookahead(),
			lookahead_mode(mode_)
		{
			advance(mode_);
		}


		std::pair<decltype(start), decltype(str)&> get_ptrs() {
			return { start, str };
		}

		const wpp::Token& peek(int mode = modes::normal) {
			// If the current mode is different from the lookahead mode
			// then we update the lookahead token and set the new
			// lookahead mode.
			if (mode != lookahead_mode) {
				str = lookahead.view.ptr; // Reset pointer to beginning of lookahead token.

				lookahead = next_token(mode);
				lookahead_mode = mode;
			}

			return lookahead;
		}

		char next(int n = 1) {
			char c = *str;
			str += n;
			return c;
		}

		char prev(int n = 1) {
			char c = *str;
			str -= n;
			return c;
		}

		wpp::Token advance(int mode = modes::normal) {
			auto tok = peek(mode);
			lookahead = next_token(mode);
			return tok;
		}

		wpp::Position position(int line_offset = 0, int column_offset = 0) const {
			return wpp::position(fname, start, lookahead.view.ptr, line_offset, column_offset);
		}

		wpp::Token next_token(int mode = modes::normal);
	};
}

#endif
