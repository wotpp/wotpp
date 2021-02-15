#pragma once

#ifndef WOTPP_LEXER
#define WOTPP_LEXER

#include <structures/token.hpp>
#include <structures/position.hpp>
#include <utils/char.hpp>
#include <exception.hpp>

#include <tinge.hpp>

namespace wpp {
	namespace modes {
		enum {
			normal,
			string,
			character,
		};
	}
}

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
		TOKEN(TOKEN_COMMA) \
		\
		TOKEN(TOKEN_IDENTIFIER) \
		TOKEN(TOKEN_PREFIX) \
		TOKEN(TOKEN_LET) \
		TOKEN(TOKEN_RUN) \
		TOKEN(TOKEN_FILE) \
		TOKEN(TOKEN_EVAL) \
		TOKEN(TOKEN_ASSERT) \
		TOKEN(TOKEN_ERROR) \
		TOKEN(TOKEN_PIPE) \
		TOKEN(TOKEN_SOURCE) \
		TOKEN(TOKEN_ESCAPE) \
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



	#define TOKEN(x) x,
		enum { TOKEN_TYPES };
	#undef TOKEN

	#define TOKEN(x) #x,
		constexpr const char* to_str[] = { TOKEN_TYPES };
	#undef TOKEN

	#undef TOKEN_TYPES
}

namespace wpp {
	class Lexer;

	void lex_quote(wpp::Lexer& lex, wpp::Token& tok);
	void lex_doublequote(wpp::Lexer& lex, wpp::Token& tok);
	void lex_comment(wpp::Lexer& lex, wpp::Token& tok);
	void lex_smart(wpp::Lexer& lex, wpp::Token& tok);
	void lex_bin(wpp::Lexer& lex, wpp::Token& tok);
	void lex_hex(wpp::Lexer& lex, wpp::Token& tok);
	void lex_cat(wpp::Lexer& lex, wpp::Token& tok);
	void lex_comma(wpp::Lexer& lex, wpp::Token& tok);
	void lex_lparen(wpp::Lexer& lex, wpp::Token& tok);
	void lex_rparen(wpp::Lexer& lex, wpp::Token& tok);
	void lex_lbrace(wpp::Lexer& lex, wpp::Token& tok);
	void lex_rbrace(wpp::Lexer& lex, wpp::Token& tok);
	void lex_whitespace(wpp::Lexer& lex, wpp::Token& tok);
	void lex_identifier(wpp::Lexer& lex, wpp::Token& tok);
	void lex_string_escape(wpp::Lexer& lex, wpp::Token& tok);
	void lex_string_other(wpp::Lexer& lex, wpp::Token& tok);
	void lex_mode_string(wpp::Lexer& lex, wpp::Token& tok);
	void lex_mode_normal(wpp::Lexer& lex, wpp::Token& tok);
	void lex_mode_character(wpp::Lexer& lex, wpp::Token& tok);
}

namespace wpp {
	struct Lexer {
		const char* const start = nullptr;
		const char* str = nullptr;

		wpp::Token lookahead{};
		int lookahead_mode = modes::normal;


		Lexer(const char* const str_, int mode_ = modes::normal):
			start(str_),
			str(str_),
			lookahead(),
			lookahead_mode(mode_)
		{
			advance(mode_);
		}


		const wpp::Token& peek(int mode = modes::normal) {
			if (mode != lookahead_mode) {
				str = lookahead.view.ptr; // reset pointer.
				lookahead = next_token(mode);
				lookahead_mode = mode;
			}

			return lookahead;
		}

		void next(int n = 1) {
			str += n;
		}

		void prev(int n = 1) {
			str -= n;
		}

		wpp::Token advance(int mode = modes::normal) {
			auto tok = peek(mode);
			lookahead = next_token(mode);
			return tok;
		}

		wpp::Position position(int line_offset = 0, int column_offset = 0) const {
			return wpp::position(start, lookahead.view.ptr, line_offset, column_offset);
		}

		wpp::Token next_token(int mode = modes::normal) {
			wpp::Token tok{{ str, 1 }, TOKEN_NONE};

			auto& [view, type] = tok;
			auto& [vptr, vlen] = view;

			while (true) {
				// we dont need to update vlen or type because
				// all of the cases where we continue dont actually
				// alter them from their initial state.
				vptr = str;

				if (*str == '\0')
					type = TOKEN_EOF;

				else if (mode == modes::character)
					lex_mode_character(*this, tok);

				else if (*str == '\'')
					lex_quote(*this, tok);

				else if (*str == '"')
					lex_doublequote(*this, tok);

				else if (mode == modes::normal) {
					if (*str == '#' and *(str + 1) == '[') {
						lex_comment(*this, tok);
						continue;
					}

					else if (wpp::is_whitespace(*str)) {
						lex_whitespace(*this, tok);
						continue;
					}

					lex_mode_normal(*this, tok);
				}

				else if (mode == modes::string)
					lex_mode_string(*this, tok);

				break;
			}

			// tinge::warnln(wpp::to_str[tok.type], " ", tok);

			return tok;
		}
	};
}

namespace wpp {
	void lex_quote(wpp::Lexer& lex, wpp::Token& tok) {
		lex.next();
		tok.type = TOKEN_QUOTE;
	}


	void lex_doublequote(wpp::Lexer& lex, wpp::Token& tok) {
		lex.next();
		tok.type = TOKEN_DOUBLEQUOTE;
	}


	void lex_comment(wpp::Lexer& lex, wpp::Token&) {
		lex.next(2);

		int depth = 1;

		while (depth > 0 and *lex.str != '\0') {
			if (*lex.str == '#' and *(lex.str + 1) == '[') {
				depth++;
				lex.next(2);
			}

			else if (*lex.str == ']') {
				depth--;
				lex.next();
			}

			else {
				lex.next();
			}
		}

		if (*lex.str == '\0' and depth != 0) {
			throw wpp::Exception{wpp::position(lex.start, lex.str), "unterminated comment."};
		}
	}


	void lex_smart(wpp::Lexer& lex, wpp::Token& tok) {
		++lex.str;
		tok.type = TOKEN_SMART;

		const char user_delim = *lex.str;
		++lex.str;

		if (not wpp::in_group(*lex.str, '\'', '"')) {
			lex.str = tok.view.ptr;
			// goto handle_ident;
			lex_identifier(lex, tok);
		}

		if (wpp::is_whitespace(user_delim))
			throw wpp::Exception{ wpp::position(lex.start, lex.str), "user defined delimiter cannot be whitespace." };
	}


	void lex_bin(wpp::Lexer& lex, wpp::Token& tok) {
		lex.next(2);
		tok.type = TOKEN_BIN;

		tok.view.ptr = lex.str;

		while (wpp::is_bin(*lex.str) or *lex.str == '_') {
			lex.next();
		}

		tok.view.length = lex.str - tok.view.ptr;
	}


	void lex_hex(wpp::Lexer& lex, wpp::Token& tok) {
		lex.next(2);
		tok.type = TOKEN_HEX;

		tok.view.ptr = lex.str;

		while (wpp::is_hex(*lex.str) or *lex.str == '_') {
			++lex.str;
		}

		tok.view.length = lex.str - tok.view.ptr;
	}


	void lex_cat(wpp::Lexer& lex, wpp::Token& tok) {
		lex.next(2);
		tok.type = TOKEN_CAT;
		tok.view.length = lex.str - tok.view.ptr;
	}


	void lex_comma(wpp::Lexer& lex, wpp::Token& tok) {
		lex.next();
		tok.type = TOKEN_COMMA;
	}


	void lex_lparen(wpp::Lexer& lex, wpp::Token& tok) {
		lex.next();
		tok.type = TOKEN_LPAREN;
	}


	void lex_rparen(wpp::Lexer& lex, wpp::Token& tok) {
		lex.next();
		tok.type = TOKEN_RPAREN;
	}


	void lex_lbrace(wpp::Lexer& lex, wpp::Token& tok) {
		lex.next();
		tok.type = TOKEN_LBRACE;
	}


	void lex_rbrace(wpp::Lexer& lex, wpp::Token& tok) {
		lex.next();
		tok.type = TOKEN_RBRACE;
	}


	void lex_whitespace(wpp::Lexer& lex, wpp::Token&) {
		do {
			lex.next();
		} while (wpp::is_whitespace(*lex.str));
	}


	void lex_identifier(wpp::Lexer& lex, wpp::Token& tok) {
		tok.type = TOKEN_IDENTIFIER;

		while (
			// make sure we don't run into a character that belongs to a token above.
			not wpp::is_whitespace(*lex.str) and
			not wpp::in_group(*lex.str, '(', ')', '{', '}', ',', '\0', '\'', '"') and
			not (*lex.str == '.' and *(lex.str + 1) == '.') and
			not (*lex.str == '#' and *(lex.str + 1) == '[')
		) {
			lex.next();
		}

		tok.view.length = lex.str - tok.view.ptr;

		if      (tok.view == "let")       tok.type = TOKEN_LET;
		else if (tok.view == "prefix")    tok.type = TOKEN_PREFIX;
		else if (tok.view == "run")       tok.type = TOKEN_RUN;
		else if (tok.view == "eval")      tok.type = TOKEN_EVAL;
		else if (tok.view == "file")      tok.type = TOKEN_FILE;
		else if (tok.view == "assert")    tok.type = TOKEN_ASSERT;
		else if (tok.view == "pipe")      tok.type = TOKEN_PIPE;
		else if (tok.view == "error")     tok.type = TOKEN_ERROR;
		else if (tok.view == "source")    tok.type = TOKEN_SOURCE;
		else if (tok.view == "escape")    tok.type = TOKEN_ESCAPE;
	}


	void lex_string_escape(wpp::Lexer& lex, wpp::Token& tok) {
		lex.next();
		tok.type = TOKEN_BACKSLASH;

		if      (*lex.str == '\\') { ++lex.str; tok.type = TOKEN_ESCAPE_BACKSLASH; }
		else if (*lex.str == '\'') { ++lex.str; tok.type = TOKEN_ESCAPE_QUOTE; }
		else if (*lex.str == '"')  { ++lex.str; tok.type = TOKEN_ESCAPE_DOUBLEQUOTE; }
		else if (*lex.str == 't')  { ++lex.str; tok.type = TOKEN_ESCAPE_TAB; }
		else if (*lex.str == 'n')  { ++lex.str; tok.type = TOKEN_ESCAPE_NEWLINE; }
		else if (*lex.str == 'r')  { ++lex.str; tok.type = TOKEN_ESCAPE_CARRIAGERETURN; }

		else if (*lex.str == 'x') {
			lex.next();
			tok.type = TOKEN_ESCAPE_HEX;

			tok.view.ptr = lex.str;

			uint8_t first_nibble = *lex.str++;
			uint8_t second_nibble = *lex.str++;

			if (not wpp::is_hex(first_nibble) or not wpp::is_hex(second_nibble))
				throw wpp::Exception{wpp::position(lex.start, tok.view.ptr), "invalid character in hex escape."};
		}

		else if (*lex.str == 'b') {
			lex.next();
			tok.type = TOKEN_ESCAPE_BIN;

			tok.view.ptr = lex.str;

			for (; lex.str != tok.view.ptr + 8; lex.next()) {
				if (not wpp::is_bin(*lex.str))
					throw wpp::Exception{wpp::position(lex.start, tok.view.ptr), "invalid character in bin escape."};
			}
		}

		tok.view.length = lex.str - tok.view.ptr;
	}


	void lex_string_other(wpp::Lexer& lex, wpp::Token& tok) {
		tok.type = TOKEN_STRING;

		while (not wpp::in_group(*lex.str, '\\', '"', '\'', '\0'))
			++lex.str;

		tok.view.length = lex.str - tok.view.ptr;
	}


	void lex_mode_string(wpp::Lexer& lex, wpp::Token& tok) {
		if (*lex.str == '\\')
			lex_string_escape(lex, tok);

		else
			lex_string_other(lex, tok);
	}


	void lex_mode_normal(wpp::Lexer& lex, wpp::Token& tok) {
		if (wpp::in_group(*lex.str, 'p', 'r', 'c'))
			lex_smart(lex, tok);

		else if (*lex.str == '0' and *(lex.str + 1) == 'b')
			lex_bin(lex, tok);

		else if (*lex.str == '0' and *(lex.str + 1) == 'x')
			lex_hex(lex, tok);

		else if (*lex.str == '.' and *(lex.str + 1) == '.')
			lex_cat(lex, tok);

		else if (*lex.str == ',')
			lex_comma(lex, tok);

		else if (*lex.str == '(')
			lex_lparen(lex, tok);

		else if (*lex.str == ')')
			lex_rparen(lex, tok);

		else if (*lex.str == '{')
			lex_lbrace(lex, tok);

		else if (*lex.str == '}')
			lex_rbrace(lex, tok);

		else
			lex_identifier(lex, tok);
	}


	void lex_mode_character(wpp::Lexer& lex, wpp::Token& tok) {
		lex.next();
		tok.type = TOKEN_CHAR;
	}
}

#endif
