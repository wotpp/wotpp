#include <misc/util/util.hpp>
#include <frontend/token.hpp>
#include <frontend/char.hpp>
#include <frontend/lexer/lexer.hpp>


namespace wpp {
	namespace {
		void lex_literal(wpp::token_type_t, bool(*)(const char*), wpp::Lexer&, wpp::Token&);
		void lex_simple(wpp::token_type_t, int, wpp::Lexer&, wpp::Token&);

		void lex_comment(wpp::Lexer&, wpp::Token&);
		void lex_single_comment(wpp::Lexer&, wpp::Token&);
		void lex_whitespace(wpp::Lexer&, wpp::Token&);

		void lex_identifier(wpp::Lexer&, wpp::Token&);
		void lex_int(wpp::Lexer&, wpp::Token&);
		void lex_stringify(wpp::Lexer&, wpp::Token&);

		void lex_smart(wpp::Lexer&, wpp::Token&);
		void lex_string_escape(wpp::Lexer&, wpp::Token&);

		void lex_mode_normal(wpp::Lexer&, wpp::Token&);
	}
}


namespace wpp {
	wpp::Token Lexer::next_token(wpp::lexer_mode_type_t mode) {
		DBG();

		wpp::Token tok{{ ptr, 1 }, TOKEN_NONE};

		auto& [view, type] = tok;
		auto& [vptr, vlen] = view;

		// Loop until we find a valid token.
		// This skips comments and whitespace.
		while (true) {
			// Update view pointer so if the lexer continues (whitespace/comment),
			// the token starts at the right location.
			vptr = ptr;

			// EOF
			if (*ptr == '\0')
				type = TOKEN_EOF;

			// Character mode: Return the next character.
			else if (mode == lexer_modes::chr) {
				type = TOKEN_CHAR;
				next();
			}

			// Handle quotes.
			else if (*ptr == '\'')
				lex_simple(TOKEN_QUOTE, 1, *this, tok);

			else if (*ptr == '"')
				lex_simple(TOKEN_DOUBLEQUOTE, 1, *this, tok);

			// Strings
			else if (mode == lexer_modes::string) {
				if (*ptr == '\\')
					lex_string_escape(*this, tok);

				else {
					type = TOKEN_STRING;

					// Consume all characters except quotes, escapes and EOF.
					while (not wpp::in_group(ptr, '\\', '"', '\'', '\0'))
						next();

					// Set view length equal to the number of consumed characters.
					vlen = ptr - vptr;
				}
			}

			else if (mode == lexer_modes::string_raw) {
				type = TOKEN_STRING;

				// Consume all characters except quotes, escapes and EOF.
				while (not wpp::in_group(ptr, '"', '\'', '\0'))
					next();

				// Set view length equal to the number of consumed characters.
				vlen = ptr - vptr;
			}

			else if (mode == lexer_modes::string_para or mode == lexer_modes::string_code) {
				// Important that this comes before the general whitespace handling.
				if (*ptr == '\n') {
					type = TOKEN_WHITESPACE_NEWLINE;
					do { next(); } while (*ptr == '\n');
					vlen = ptr - vptr;
				}

				else if (wpp::in_group(ptr, ' ', '\t', '\r', '\v', '\f') or wpp::is_whitespace_utf8(ptr)) {
					type = TOKEN_WHITESPACE;
					do { next(); } while (wpp::in_group(ptr, ' ', '\t', '\r', '\v', '\f') or wpp::is_whitespace_utf8(ptr));
					vlen = ptr - vptr;
				}

				else if (*ptr == '\\')
					lex_string_escape(*this, tok);

				else {
					type = TOKEN_STRING;

					// Consume all characters except quotes, escapes and EOF.
					while (not (wpp::in_group(ptr, '\\', '"', '\'', '\0') or wpp::is_whitespace(ptr)))
						next();

					// Set view length equal to the number of consumed characters.
					vlen = ptr - vptr;
				}
			}

			else if (mode == lexer_modes::stringify)
				lex_stringify(*this, tok);

			else if (*ptr == '#' and *(ptr + 1) == '[') {
				lex_comment(*this, tok);
				continue;
			}

			else if (wpp::is_whitespace(ptr)) {
				lex_whitespace(*this, tok);
				continue; // Throw away whitespace and get next token.
			}

			// Normal mode.
			else if (mode == lexer_modes::slice) {
				if (*ptr == ':')
					lex_simple(TOKEN_COLON, 1, *this, tok);

				else if (*ptr == ']')
					lex_simple(TOKEN_RBRACKET, 1, *this, tok);

				else if (*ptr == '[')
					lex_simple(TOKEN_LBRACKET, 1, *this, tok);

				else if (wpp::is_digit(ptr) or *ptr == '-')
					lex_int(*this, tok);
			}

			else if (mode == lexer_modes::normal) {
				// If neither comment nor whitespace, handle a normal token.
				lex_mode_normal(*this, tok);
			}

			// Break by default. We use continue above if we need another token.
			break;
		}

		return tok;
	}
}


namespace wpp {
	namespace {
		void lex_comment(wpp::Lexer& lex, wpp::Token& tok) {
			DBG();

			auto& ptr = lex.ptr;

			auto& [view, type] = tok;
			auto& [vptr, vlen] = view;

			const wpp::Pos start_pos = lex.position();

			lex.next(2); // skip `#[`

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
				wpp::error(start_pos, lex.env, "unterminated comment", "reached EOF while parsing multiline comment that begins here");

			// Update view pointer so when the lexer continues, the token starts
			// at the right location.
			vptr = ptr;
		}


		void lex_smart(wpp::Lexer& lex, wpp::Token& tok) {
			DBG();

			auto& ptr = lex.ptr;

			auto& [view, type] = tok;
			auto& [vptr, vlen] = view;

			char str_type = lex.next();

			if (str_type == 'r')
				type = TOKEN_RAWSTR;

			else if (str_type == 'p')
				type = TOKEN_PARASTR;

			else if (str_type == 'c')
				type = TOKEN_CODESTR;

			// Get user delimiter. 'r#"'
			//                       ^
			const char* user_delim = ptr;
			lex.next();

			// Make sure there's a quote here, if there isn't, we've made
			// a mistake and this is actually an identifier.
			// We call lex_identifier to perform the correct action.
			if (not wpp::is_quote(ptr) or wpp::is_whitespace(user_delim)) {
				lex.ptr = vptr; // Reset pointer to where it was before this function.
				lex_identifier(lex, tok);
			}
		}


		void lex_literal(wpp::token_type_t type, bool(*predicate)(const char*), wpp::Lexer& lex, wpp::Token& tok) {
			DBG();

			lex.next(2);
			tok.type = type;

			// Token view skips past the leading 0b or 0x.
			tok.view.ptr = lex.ptr;

			// Consume digits while predicate is satisfied.
			while (predicate(lex.ptr) or *lex.ptr == '_')
				lex.next();

			// Set token view length to the number of consumed characters.
			tok.view.length = lex.ptr - tok.view.ptr;
		}


		// Handle simple tokens which are just a couple of characters.
		void lex_simple(wpp::token_type_t type, int n, wpp::Lexer& lex, wpp::Token& tok) {
			DBG();

			lex.next(n);
			tok.type = type;
		}


		void lex_whitespace(wpp::Lexer& lex, wpp::Token& tok) {
			DBG();

			tok.type = TOKEN_WHITESPACE;

			// Consume as much whitespace as we can.
			do
				lex.next();
			while (wpp::is_whitespace(lex.ptr));

			// Set token view length to the number of consumed characters.
			tok.view.length = lex.ptr - tok.view.ptr;
		}


		void lex_int(wpp::Lexer& lex, wpp::Token& tok) {
			DBG();

			auto& ptr = lex.ptr;

			auto& [view, type] = tok;
			auto& [vptr, vlen] = view;

			type = TOKEN_INT;

			do
				lex.next();
			while (wpp::is_digit(ptr));

			vlen = ptr - vptr;
		}


		void lex_stringify(wpp::Lexer& lex, wpp::Token& tok) {
			DBG();

			auto& ptr = lex.ptr;

			auto& [view, type] = tok;
			auto& [vptr, vlen] = view;

			type = TOKEN_STRING;

			do
				lex.next();
			while (
				not wpp::is_whitespace(ptr) and
				not wpp::is_grouping(ptr) and
				*ptr != '\0'
			);

			vlen = ptr - vptr;
		}


		void lex_identifier(wpp::Lexer& lex, wpp::Token& tok) {
			DBG();

			auto& ptr = lex.ptr;

			auto& [view, type] = tok;
			auto& [vptr, vlen] = view;

			type = TOKEN_IDENTIFIER;

			// Make sure we don't run into a character that belongs to another token.
			do
				lex.next();
			while (
				not wpp::is_whitespace(ptr) and
				not wpp::is_grouping(ptr) and
				*ptr != '\0'
			);

			// Set length to the number of consumed characters.
			vlen = ptr - vptr;

			// Check if consumed string is actually a keyword.
			if      (view == "let")    type = TOKEN_LET;
			else if (view == "match")  type = TOKEN_MATCH;
			else if (view == "pop")    type = TOKEN_POP;
			else if (view == "drop")   type = TOKEN_DROP;
			else if (view == "new")    type = TOKEN_NEW;

			else if (view == "use")    type = TOKEN_INTRINSIC_USE;
			else if (view == "run")    type = TOKEN_INTRINSIC_RUN;
			else if (view == "file")   type = TOKEN_INTRINSIC_FILE;
			else if (view == "assert") type = TOKEN_INTRINSIC_ASSERT;
			else if (view == "pipe")   type = TOKEN_INTRINSIC_PIPE;
			else if (view == "error")  type = TOKEN_INTRINSIC_ERROR;
			else if (view == "log")    type = TOKEN_INTRINSIC_LOG;
		}


		void lex_string_escape(wpp::Lexer& lex, wpp::Token& tok) {
			DBG();

			auto& ptr = lex.ptr;

			auto& [view, type] = tok;
			auto& [vptr, vlen] = view;

			lex.next();
			type = TOKEN_BACKSLASH;

			// Handle basic escapes.
			if      (*ptr == '\\') { lex.next(); type = TOKEN_ESCAPE_BACKSLASH; }
			else if (*ptr == '\'') { lex.next(); type = TOKEN_ESCAPE_QUOTE; }
			else if (*ptr == '"')  { lex.next(); type = TOKEN_ESCAPE_DOUBLEQUOTE; }
			else if (*ptr == 't')  { lex.next(); type = TOKEN_ESCAPE_TAB; }
			else if (*ptr == 'n')  { lex.next(); type = TOKEN_ESCAPE_NEWLINE; }
			else if (*ptr == 'r')  { lex.next(); type = TOKEN_ESCAPE_CARRIAGERETURN; }

			// Hex escape \xFF.
			else if (*ptr == 'x') {
				lex.next();
				type = TOKEN_ESCAPE_HEX;
				vptr = ptr;

				// Check if nibbles are valid digits.
				if (not wpp::is_hex(ptr))
					wpp::error(wpp::Pos{lex.env.sources.top(), wpp::View{lex.ptr, 1}}, lex.env,
						"invalid character",
						"invalid character in hex escape sequence"
					);

				lex.next();

				if (not wpp::is_hex(ptr))
					wpp::error(wpp::Pos{lex.env.sources.top(), wpp::View{lex.ptr, 1}}, lex.env,
						"invalid character",
						"invalid character in hex escape sequence"
					);

				lex.next();
			}

			// Bin escape \b00001111.
			else if (*ptr == 'b') {
				lex.next();
				type = TOKEN_ESCAPE_BIN;
				vptr = ptr;

				// Consume 8 characters, check if each one is a valid digit.
				for (; ptr != vptr + 8; lex.next()) {
					if (not wpp::is_bin(ptr))
						wpp::error(wpp::Pos{lex.env.sources.top(), wpp::View{lex.ptr, 1}}, lex.env,
							"invalid character",
							"invalid character in binary escape sequence"
						);
				}
			}

			// Set view length to the number of consumed characters.
			vlen = ptr - vptr;
		}


		void lex_mode_normal(wpp::Lexer& lex, wpp::Token& tok) {
			DBG();

			if (wpp::in_group(lex.ptr, 'p', 'r', 'c'))
				lex_smart(lex, tok);

			else if (*lex.ptr == '0' and *(lex.ptr + 1) == 'b')
				lex_literal(TOKEN_BIN, wpp::is_bin, lex, tok);

			else if (*lex.ptr == '0' and *(lex.ptr + 1) == 'x')
				lex_literal(TOKEN_HEX, wpp::is_hex, lex, tok);

			else if (*lex.ptr == '.' and *(lex.ptr + 1) == '.')
				lex_simple(TOKEN_CAT, 2, lex, tok);

			else if (*lex.ptr == '-' and *(lex.ptr + 1) == '>')
				lex_simple(TOKEN_ARROW, 2, lex, tok);

			else if (*lex.ptr == '|')
				lex_simple(TOKEN_BAR, 1, lex, tok);

			else if (*lex.ptr == '!')
				lex_simple(TOKEN_EVAL, 1, lex, tok);

			else if (*lex.ptr == '\\')
				lex_simple(TOKEN_STRINGIFY, 1, lex, tok);

			else if (*lex.ptr == '*')
				lex_simple(TOKEN_STAR, 1, lex, tok);

			else if (*lex.ptr == '(')
				lex_simple(TOKEN_LPAREN, 1, lex, tok);

			else if (*lex.ptr == ')')
				lex_simple(TOKEN_RPAREN, 1, lex, tok);

			else if (*lex.ptr == '[')
				lex_simple(TOKEN_LBRACKET, 1, lex, tok);

			else if (*lex.ptr == ']')
				lex_simple(TOKEN_RBRACKET, 1, lex, tok);

			else if (*lex.ptr == '{')
				lex_simple(TOKEN_LBRACE, 1, lex, tok);

			else if (*lex.ptr == '}')
				lex_simple(TOKEN_RBRACE, 1, lex, tok);

			else
				lex_identifier(lex, tok);
		}
	}
}
