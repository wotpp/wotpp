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
		TOKEN(TOKEN_OTHER) \
		\
		TOKEN(TOKEN_WHITESPACE) \
		TOKEN(TOKEN_WHITESPACE_SPACE) \
		TOKEN(TOKEN_WHITESPACE_TAB) \
		TOKEN(TOKEN_WHITESPACE_CARRIAGERETURN) \
		TOKEN(TOKEN_WHITESPACE_NEWLINE) \
		\
		TOKEN(TOKEN_BACKSLASH) \
		TOKEN(TOKEN_CAT) \
		TOKEN(TOKEN_ARROW) \
		TOKEN(TOKEN_STAR) \
		TOKEN(TOKEN_BAR) \
		TOKEN(TOKEN_STRINGIFY) \
		TOKEN(TOKEN_EVAL) \
		TOKEN(TOKEN_COLON) \
		TOKEN(TOKEN_COMMA) \
		\
		TOKEN(TOKEN_IDENTIFIER) \
		TOKEN(TOKEN_INT) \
		TOKEN(TOKEN_MATCH) \
		TOKEN(TOKEN_LET) \
		TOKEN(TOKEN_DROP) \
		TOKEN(TOKEN_POP) \
		TOKEN(TOKEN_NEW) \
		\
		TOKEN(TOKEN_INTRINSIC_USE) \
		TOKEN(TOKEN_INTRINSIC_RUN) \
		TOKEN(TOKEN_INTRINSIC_FILE) \
		TOKEN(TOKEN_INTRINSIC_ASSERT) \
		TOKEN(TOKEN_INTRINSIC_ERROR) \
		TOKEN(TOKEN_INTRINSIC_PIPE) \
		TOKEN(TOKEN_INTRINSIC_LOG) \
		\
		TOKEN(TOKEN_LPAREN) \
		TOKEN(TOKEN_RPAREN) \
		TOKEN(TOKEN_LBRACE) \
		TOKEN(TOKEN_RBRACE) \
		TOKEN(TOKEN_LBRACKET) \
		TOKEN(TOKEN_RBRACKET) \
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
		wpp::Env& env;
		const char* ptr = nullptr;

		wpp::Token lookahead{};
		wpp::lexer_mode_type_t lookahead_mode = lexer_modes::normal;


		Lexer(
			wpp::Env& env_,
			wpp::lexer_mode_type_t mode_ = lexer_modes::normal
		):
			env(env_),
			ptr(env_.sources.top().base),
			lookahead({ptr, 1}, TOKEN_NONE),
			lookahead_mode(mode_)
		{
			if (not wpp::validate_utf8(ptr)) {
				wpp::error(report_modes::encoding, wpp::Pos{env.sources.top(), wpp::View{ ptr, 1 }}, env,
					"invalid UTF-8",
					"malformed bytes appear in source"
				);

				env.state |=
					wpp::ERROR_MODE_UTF8 &
					wpp::ABORT_ERROR_RECOVERY &
					wpp::ABORT_EVALUATION;
			}

			ptr = env_.sources.top().base;

			advance(mode_);
		}


		wpp::Pos position() const {
			DBG();
			return { env.sources.top(), lookahead.view };
		}


		const wpp::Token& peek(wpp::lexer_mode_type_t mode = lexer_modes::normal) {
			DBG();

			// If the current mode is different from the lookahead mode
			// then we update the lookahead token and set the new
			// lookahead mode.
			if (mode != lookahead_mode) {
				DBG(detail::lookup_colour_enabled(ANSI_FG_RED), lexer_modes::lexer_mode_to_str[lookahead_mode], " -> ", lexer_modes::lexer_mode_to_str[mode]);

				ptr = lookahead.view.ptr; // Reset pointer to beginning of lookahead token.

				lookahead = next_token_wrapper(mode);
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
			DBG();

			auto tok = peek(mode);
			lookahead = next_token_wrapper(mode);
			return tok;
		}

		// Wrap next_token to catch lexer errors and set the error state
		// before propagating the error.
		wpp::Token next_token_wrapper(wpp::lexer_mode_type_t mode = lexer_modes::normal) {
			try {
				return next_token(mode);
			}

			catch (const wpp::Report& e) {
				env.state |=
					wpp::ERROR_MODE_LEX &
					wpp::ABORT_EVALUATION &
					wpp::ABORT_ERROR_RECOVERY;

				throw;
			}
		}

		wpp::Token next_token(wpp::lexer_mode_type_t mode = lexer_modes::normal);
	};
}

#endif
