#pragma once

#ifndef WOTPP_LEXER
#define WOTPP_LEXER

#include <string>
#include <utility>

#include <misc/fwddecl.hpp>
#include <misc/util/util.hpp>
#include <frontend/token.hpp>
#include <frontend/char.hpp>
#include <structures/environment.hpp>

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
		TOKEN(TOKEN_STRINGIFY) \
		TOKEN(TOKEN_EVAL) \
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
		\
		TOKEN(TOKEN_RAWSTR) \
		TOKEN(TOKEN_CODESTR) \
		TOKEN(TOKEN_PARASTR) \
		\
		TOKEN(TOKEN_TOTAL)

	#define TOKEN(x) x,
		enum: token_type_t { TOKEN_TYPES };
	#undef TOKEN

	#define TOKEN(x) #x,
		constexpr const char* token_to_str[] = { TOKEN_TYPES };
	#undef TOKEN

	#undef TOKEN_TYPES
}


namespace wpp {
	struct Lexer {
		const wpp::Env& env;
		const char* ptr = nullptr;

		wpp::Token lookahead{};
		wpp::lexer_mode_type_t lookahead_mode = lexer_modes::normal;


		Lexer(
			const wpp::Env& env_,
			wpp::lexer_mode_type_t mode_ = lexer_modes::normal
		):
			env(env_),
			ptr(env_.sources.top().base),
			lookahead({ptr, 1}, TOKEN_NONE),
			lookahead_mode(mode_)
		{
			if (not wpp::validate_utf8(ptr))
				wpp::error_utf8(wpp::Pos{env.sources.top(), wpp::View{ ptr, 1 }}, env,
					"invalid UTF-8",
					"invalid bytes in source"
				);

			ptr = env_.sources.top().base;

			advance(mode_);
		}


		wpp::Pos position() const {
			return { env.sources.top(), lookahead.view };
		}


		const wpp::Token& peek(wpp::lexer_mode_type_t mode = lexer_modes::normal) {
			// If the current mode is different from the lookahead mode
			// then we update the lookahead token and set the new
			// lookahead mode.
			if (mode != lookahead_mode) {
				DBG(ANSI_FG_RED, lexer_modes::lexer_mode_to_str[lookahead_mode], " -> ", lexer_modes::lexer_mode_to_str[mode]);

				ptr = lookahead.view.ptr; // Reset pointer to beginning of lookahead token.

				lookahead = next_token(mode);
				lookahead_mode = mode;
			}

			return lookahead;
		}

		char next(int n = 1) {
			char c = *ptr;

			while (n--)
				ptr += wpp::size_utf8(ptr);

			return c;
		}

		wpp::Token advance(wpp::lexer_mode_type_t mode = lexer_modes::normal) {
			auto tok = peek(mode);
			lookahead = next_token(mode);
			return tok;
		}

		wpp::Token next_token(wpp::lexer_mode_type_t mode = lexer_modes::normal);
	};
}

#endif
