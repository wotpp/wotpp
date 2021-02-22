#pragma once

#ifndef WOTPP_LEXER
#define WOTPP_LEXER

#include <structures/token.hpp>
#include <structures/position.hpp>
#include <utils/char.hpp>
#include <exception.hpp>

#include <tinge.hpp>

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
		TOKEN(TOKEN_BACKTICK) \
		\
		TOKEN(TOKEN_IDENTIFIER) \
		TOKEN(TOKEN_MAP) \
		TOKEN(TOKEN_PREFIX) \
		TOKEN(TOKEN_LET) \
		TOKEN(TOKEN_DROP) \
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
	struct Lexer;

	inline void lex_literal(wpp::token_type_t, bool(*)(char), wpp::Lexer&, wpp::Token&);
	inline void lex_simple(wpp::token_type_t, int, wpp::Lexer&, wpp::Token&);

	inline void lex_comment(wpp::Lexer&, wpp::Token&);
	inline void lex_single_comment(wpp::Lexer&, wpp::Token&);
	inline void lex_whitespace(wpp::Lexer&, wpp::Token&);

	inline void lex_identifier(wpp::Lexer&, wpp::Token&);

	inline void lex_smart(wpp::Lexer&, wpp::Token&);
	inline void lex_string_escape(wpp::Lexer&, wpp::Token&);
	inline void lex_string_other(wpp::Lexer&, wpp::Token&);

	inline void lex_mode_string(wpp::Lexer&, wpp::Token&);
	inline void lex_mode_normal(wpp::Lexer&, wpp::Token&);
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

		wpp::Token next_token(int mode = modes::normal) {
			wpp::Token tok{{ str, 1 }, TOKEN_NONE};

			// Loop until we find a valid token.
			// This skips comments and whitespace.
			while (true) {
				// EOF
				if (*str == '\0')
					tok.type = TOKEN_EOF;

				// Character mode: Return the next character.
				else if (mode == modes::character) {
					next();
					tok.type = TOKEN_CHAR;
				}

				// Handle quotes.
				else if (*str == '\'')
					lex_simple(TOKEN_QUOTE, 1, *this, tok);

				else if (*str == '"')
					lex_simple(TOKEN_DOUBLEQUOTE, 1, *this, tok);

				// Normal mode.
				else if (mode == modes::normal) {
					// Comment.
					if (*str == '#' and *(str + 1) == '[') {
						lex_comment(*this, tok);
						continue; // Throw away comment and get next token.
					}

					else if (*str == '#') {
						lex_single_comment(*this, tok);
						continue;
					}

					else if (wpp::is_whitespace(*str)) {
						lex_whitespace(*this, tok);
						continue; // Throw away whitespace and get next token.
					}

					// If neither comment nor token, handle a normal token.
					lex_mode_normal(*this, tok);
				}

				// String mode.
				else if (mode == modes::string)
					lex_mode_string(*this, tok);

				// Break by default. We use continue above if we need another token.
				break;
			}

			return tok;
		}
	};
}

namespace wpp {
	inline void lex_comment(wpp::Lexer& lex, wpp::Token& tok) {
		auto [start, ptr] = lex.get_ptrs();

		auto& [view, type] = tok;
		auto& [vptr, vlen] = view;

		lex.next(2);

		// Track matching opening and closing tokens for comments
		// and skip comment contents.
		int depth = 1;

		while (depth > 0 and *ptr != '\0') {
			if (*ptr == '#' and *(ptr + 1) == '[')
				depth++, lex.next(2);

			else if (*ptr == ']')
				depth--, lex.next();

			else
				lex.next();
		}

		if (*ptr == '\0' and depth != 0)
			throw wpp::Exception{wpp::position(lex.fname, start, ptr), "unterminated comment."};

		// Update view pointer so when the lexer continues, the token starts
		// at the right location.
		vptr = ptr;
	}


	inline void lex_single_comment(wpp::Lexer& lex, wpp::Token& tok) {
		auto [start, ptr] = lex.get_ptrs();

		auto& [view, type] = tok;
		auto& [vptr, vlen] = view;

		lex.next();
		while (lex.next() != '\n');

		vptr = ptr;
	}


	inline void lex_smart(wpp::Lexer& lex, wpp::Token& tok) {
		// Skip 'r', 'p' or 'c'.
		++lex.str;
		tok.type = TOKEN_SMART;

		// Get user delimiter. 'r#"'
		//                       ^
		const char user_delim = *lex.str;
		++lex.str;

		// Make sure there's a quote here, if there isn't, we've made
		// a mistake and this is actually an identifier.
		// We call lex_identifier to perform the correct action.
		if (not wpp::in_group(*lex.str, '\'', '"') or wpp::is_whitespace(user_delim)) {
			lex.str = tok.view.ptr; // Reset pointer to where it was before this function.
			lex_identifier(lex, tok);
		}

		// If we are not, in fact, an identifer, check if the user delimiter is not whitespace.
		// if (wpp::is_whitespace(user_delim))
		// 	throw wpp::Exception{ wpp::position(lex.start, lex.str), "user defined delimiter cannot be whitespace." };
	}


	inline void lex_literal(wpp::token_type_t type, bool(*predicate)(char), wpp::Lexer& lex, wpp::Token& tok) {
		lex.next(2);
		tok.type = type;

		// Token view skips past the leading 0b or 0x.
		tok.view.ptr = lex.str;

		// Consume digits while predicate is satisfied.
		while (predicate(*lex.str) or *lex.str == '_')
			lex.next();

		// Set token view length to the number of consumed characters.
		tok.view.length = lex.str - tok.view.ptr;
	}


	// Handle simple tokens which are just a couple of characters.
	inline void lex_simple(wpp::token_type_t type, int n, wpp::Lexer& lex, wpp::Token& tok) {
		lex.next(n);
		tok.type = type;
	}


	inline void lex_whitespace(wpp::Lexer& lex, wpp::Token& tok) {
		// Consume as much whitespace as we can.
		do {
			lex.next();
		} while (wpp::is_whitespace(*lex.str));

		// Update view pointer so when the lexer continues, the token starts
		// at the right location.
		tok.view.ptr = lex.str;
	}


	inline void lex_identifier(wpp::Lexer& lex, wpp::Token& tok) {
		auto [start, ptr] = lex.get_ptrs();

		auto& [view, type] = tok;
		auto& [vptr, vlen] = view;

		type = TOKEN_IDENTIFIER;

		// Make sure we don't run into a character that belongs to another token.
		while (
			not wpp::is_whitespace(*ptr) and
			not wpp::in_group(*ptr, '(', ')', '{', '}', ',', '\0', '\'', '"') and
			not (*ptr == '.' and *(ptr + 1) == '.') and
			not (*ptr == '#' and *(ptr + 1) == '[')
		)
			lex.next();

		// Set length to the number of consumed characters.
		vlen = ptr - vptr;

		// Check if consumed string is actually a keyword.
		if      (view == "let")       type = TOKEN_LET;
		else if (view == "prefix")    type = TOKEN_PREFIX;
		else if (view == "map")       type = TOKEN_MAP;
		else if (view == "run")       type = TOKEN_RUN;
		else if (view == "eval")      type = TOKEN_EVAL;
		else if (view == "file")      type = TOKEN_FILE;
		else if (view == "assert")    type = TOKEN_ASSERT;
		else if (view == "pipe")      type = TOKEN_PIPE;
		else if (view == "error")     type = TOKEN_ERROR;
		else if (view == "source")    type = TOKEN_SOURCE;
		else if (view == "escape")    type = TOKEN_ESCAPE;
		else if (view == "slice")     type = TOKEN_SLICE;
		else if (view == "find")      type = TOKEN_FIND;
		else if (view == "length")    type = TOKEN_LENGTH;
		else if (view == "log")       type = TOKEN_LOG;
		else if (view == "drop")      type = TOKEN_DROP;
	}


	inline void lex_string_escape(wpp::Lexer& lex, wpp::Token& tok) {
		auto [start, ptr] = lex.get_ptrs();

		auto& [view, type] = tok;
		auto& [vptr, vlen] = view;

		lex.next();
		type = TOKEN_BACKSLASH;

		// Handle basic escapes.
		if      (*ptr == '\\') { ++ptr; type = TOKEN_ESCAPE_BACKSLASH; }
		else if (*ptr == '\'') { ++ptr; type = TOKEN_ESCAPE_QUOTE; }
		else if (*ptr == '"')  { ++ptr; type = TOKEN_ESCAPE_DOUBLEQUOTE; }
		else if (*ptr == 't')  { ++ptr; type = TOKEN_ESCAPE_TAB; }
		else if (*ptr == 'n')  { ++ptr; type = TOKEN_ESCAPE_NEWLINE; }
		else if (*ptr == 'r')  { ++ptr; type = TOKEN_ESCAPE_CARRIAGERETURN; }

		// Hex escape \xFF.
		else if (*ptr == 'x') {
			lex.next();
			tok.type = TOKEN_ESCAPE_HEX;

			view.ptr = ptr;

			// Get first and second nibble.
			uint8_t first_nibble = *ptr++;
			uint8_t second_nibble = *ptr++;

			// Check if nibbles are valid digits.
			if (not wpp::is_hex(first_nibble) or not wpp::is_hex(second_nibble))
				throw wpp::Exception{wpp::position(lex.fname, start, vptr), "invalid character in hex escape."};
		}

		// Bin escape \b00001111.
		else if (*ptr == 'b') {
			lex.next();
			type = TOKEN_ESCAPE_BIN;

			vptr = ptr;

			// Consume 8 characters, check if each one is a valid digit.
			for (; ptr != vptr + 8; lex.next()) {
				if (not wpp::is_bin(*ptr))
					throw wpp::Exception{wpp::position(lex.fname, start, vptr), "invalid character in bin escape."};
			}
		}

		// Set view length to the number of consumed characters.
		vlen = ptr - vptr;
	}


	inline void lex_string_other(wpp::Lexer& lex, wpp::Token& tok) {
		tok.type = TOKEN_STRING;

		// Consume all characters except quotes, escapes and EOF.
		while (not wpp::in_group(*lex.str, '\\', '"', '\'', '\0'))
			++lex.str;

		// Set view length equal to the number of consumed characters.
		tok.view.length = lex.str - tok.view.ptr;
	}


	inline void lex_mode_string(wpp::Lexer& lex, wpp::Token& tok) {
		if (*lex.str == '\\')
			lex_string_escape(lex, tok);

		else
			lex_string_other(lex, tok);
	}


	inline void lex_mode_normal(wpp::Lexer& lex, wpp::Token& tok) {
		if (wpp::in_group(*lex.str, 'p', 'r', 'c'))
			lex_smart(lex, tok);

		else if (*lex.str == '0' and *(lex.str + 1) == 'b')
			lex_literal(TOKEN_BIN, wpp::is_bin, lex, tok);

		else if (*lex.str == '0' and *(lex.str + 1) == 'x')
			lex_literal(TOKEN_HEX, wpp::is_hex, lex, tok);

		else if (*lex.str == '.' and *(lex.str + 1) == '.')
			lex_simple(TOKEN_CAT, 2, lex, tok);

		else if (*lex.str == '-' and *(lex.str + 1) == '>')
			lex_simple(TOKEN_ARROW, 2, lex, tok);

		else if (*lex.str == ',')
			lex_simple(TOKEN_COMMA, 1, lex, tok);

		else if (*lex.str == '|')
			lex_simple(TOKEN_BAR, 1, lex, tok);

		else if (*lex.str == '`')
			lex_simple(TOKEN_BACKTICK, 1, lex, tok);

		else if (*lex.str == '*')
			lex_simple(TOKEN_STAR, 1, lex, tok);

		else if (*lex.str == '(')
			lex_simple(TOKEN_LPAREN, 1, lex, tok);

		else if (*lex.str == ')')
			lex_simple(TOKEN_RPAREN, 1, lex, tok);

		else if (*lex.str == '{')
			lex_simple(TOKEN_LBRACE, 1, lex, tok);

		else if (*lex.str == '}')
			lex_simple(TOKEN_RBRACE, 1, lex, tok);

		else
			lex_identifier(lex, tok);
	}
}

#endif
