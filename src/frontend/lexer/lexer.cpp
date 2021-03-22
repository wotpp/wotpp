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

		void lex_smart(wpp::Lexer&, wpp::Token&);
		void lex_string_escape(wpp::Lexer&, wpp::Token&);
		void lex_string_other(wpp::Lexer&, wpp::Token&);
		void lex_string_raw(wpp::Lexer&, wpp::Token&);

		void lex_mode_string(wpp::Lexer&, wpp::Token&, bool);
		void lex_mode_normal(wpp::Lexer&, wpp::Token&);
	}
}


namespace wpp {
	wpp::Token Lexer::next_token(wpp::lexer_mode_type_t mode) {
		wpp::Token tok{{ ptr, 1 }, TOKEN_NONE};

		// Loop until we find a valid token.
		// This skips comments and whitespace.
		while (true) {
			// EOF
			if (*ptr == '\0')
				tok.type = TOKEN_EOF;

			// Character mode: Return the next character.
			else if (mode == lexer_modes::chr) {
				next();
				tok.type = TOKEN_CHAR;
			}

			// Handle quotes.
			else if (*ptr == '\'')
				lex_simple(TOKEN_QUOTE, 1, *this, tok);

			else if (*ptr == '"')
				lex_simple(TOKEN_DOUBLEQUOTE, 1, *this, tok);

			// Normal mode.
			else if (mode == lexer_modes::normal) {
				// Comment.
				if (*ptr == '#' and *(ptr + 1) == '[') {
					lex_comment(*this, tok);
					continue; // Throw away comment and get next token.
				}

				else if (*ptr == '#') {
					lex_single_comment(*this, tok);
					continue;
				}

				else if (wpp::is_whitespace(ptr)) {
					lex_whitespace(*this, tok);
					continue; // Throw away whitespace and get next token.
				}

				// If neither comment nor token, handle a normal token.
				lex_mode_normal(*this, tok);
			}

			// String mode.
			else if (mode == lexer_modes::string)
				lex_mode_string(*this, tok, true);

			else if (mode == lexer_modes::string_no_escape)
				lex_mode_string(*this, tok, false);

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


		void lex_single_comment(wpp::Lexer& lex, wpp::Token& tok) {
			DBG();

			auto& [view, type] = tok;
			auto& [vptr, vlen] = view;

			lex.next();
			while (lex.next() != '\n');

			vptr = lex.ptr;
		}


		void lex_smart(wpp::Lexer& lex, wpp::Token& tok) {
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

			DBG(token_to_str[type], ": '", wpp::View(vptr, ptr - vptr), "'");
		}


		void lex_literal(wpp::token_type_t type, bool(*predicate)(const char*), wpp::Lexer& lex, wpp::Token& tok) {
			lex.next(2);
			tok.type = type;

			// Token view skips past the leading 0b or 0x.
			tok.view.ptr = lex.ptr;

			// Consume digits while predicate is satisfied.
			while (predicate(lex.ptr) or *lex.ptr == '_')
				lex.next();

			// Set token view length to the number of consumed characters.
			tok.view.length = lex.ptr - tok.view.ptr;

			DBG(token_to_str[type], ": '", wpp::View(tok.view.ptr, lex.ptr - tok.view.ptr), "'");
		}


		// Handle simple tokens which are just a couple of characters.
		void lex_simple(wpp::token_type_t type, int n, wpp::Lexer& lex, wpp::Token& tok) {
			lex.next(n);
			tok.type = type;
			DBG(token_to_str[type], ": '", wpp::View(tok.view.ptr, lex.ptr - tok.view.ptr), "'");
		}


		void lex_whitespace(wpp::Lexer& lex, wpp::Token& tok) {
			// Consume as much whitespace as we can.
			do {
				lex.next();
			} while (wpp::is_whitespace(lex.ptr));

			// Update view pointer so when the lexer continues, the token starts
			// at the right location.
			tok.view.ptr = lex.ptr;
		}


		void lex_identifier(wpp::Lexer& lex, wpp::Token& tok) {
			auto& ptr = lex.ptr;

			auto& [view, type] = tok;
			auto& [vptr, vlen] = view;

			type = TOKEN_IDENTIFIER;

			// Make sure we don't run into a character that belongs to another token.
			while (
				not wpp::is_whitespace(ptr) and
				not wpp::in_group(ptr, '(', ')', '{', '}', ',', '\0', '\'', '"') and
				not (*ptr == '.' and *(ptr + 1) == '.') and
				not (*ptr == '#' and *(ptr + 1) == '[')
			)
				lex.next();

			// Set length to the number of consumed characters.
			vlen = ptr - vptr;

			// Check if consumed string is actually a keyword.
			if      (view == "let")       type = TOKEN_LET;
			else if (view == "map")       type = TOKEN_MAP;
			else if (view == "push")      type = TOKEN_PUSH;
			else if (view == "pop")       type = TOKEN_POP;
			else if (view == "use")       type = TOKEN_USE;
			else if (view == "run")       type = TOKEN_RUN;
			else if (view == "file")      type = TOKEN_FILE;
			else if (view == "assert")    type = TOKEN_ASSERT;
			else if (view == "pipe")      type = TOKEN_PIPE;
			else if (view == "error")     type = TOKEN_ERROR;
			else if (view == "source")    type = TOKEN_SOURCE;
			else if (view == "escape")    type = TOKEN_ESCAPE;
			else if (view == "log")       type = TOKEN_LOG;
			else if (view == "drop")      type = TOKEN_DROP;

			DBG(token_to_str[type], ": '", view, "'");
		}


		void lex_string_escape(wpp::Lexer& lex, wpp::Token& tok) {
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
			DBG(token_to_str[type], ": '", view, "'");
		}


		void lex_string_other(wpp::Lexer& lex, wpp::Token& tok) {
			tok.type = TOKEN_STRING;

			// Consume all characters except quotes, escapes and EOF.
			while (not wpp::in_group(lex.ptr, '\\', '"', '\'', '\0'))
				lex.next();

			// Set view length equal to the number of consumed characters.
			tok.view.length = lex.ptr - tok.view.ptr;

			DBG(token_to_str[tok.type], ": '", tok.view, "'");
		}


		void lex_string_raw(wpp::Lexer& lex, wpp::Token& tok) {
			tok.type = TOKEN_STRING;

			while (not wpp::in_group(lex.ptr, '"', '\'', '\0'))
				lex.next();

			tok.view.length = lex.ptr - tok.view.ptr;

			DBG(token_to_str[tok.type], ": '", tok.view, "'");
		}


		void lex_mode_string(wpp::Lexer& lex, wpp::Token& tok, bool handle_escapes) {
			if (handle_escapes and *lex.ptr == '\\')
				lex_string_escape(lex, tok);

			else if (not handle_escapes and *lex.ptr == '\\')
				lex_string_raw(lex, tok);

			else
				lex_string_other(lex, tok);
		}


		void lex_mode_normal(wpp::Lexer& lex, wpp::Token& tok) {
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

			else if (*lex.ptr == ',')
				lex_simple(TOKEN_COMMA, 1, lex, tok);

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

			else if (*lex.ptr == '{')
				lex_simple(TOKEN_LBRACE, 1, lex, tok);

			else if (*lex.ptr == '}')
				lex_simple(TOKEN_RBRACE, 1, lex, tok);

			else
				lex_identifier(lex, tok);
		}
	}
}
