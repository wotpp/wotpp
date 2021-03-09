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

		void lex_mode_string(wpp::Lexer&, wpp::Token&);
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
				lex_mode_string(*this, tok);

			// Break by default. We use continue above if we need another token.
			break;
		}

		return tok;
	}
}


namespace wpp {
	namespace {
		void lex_comment(wpp::Lexer& lex, wpp::Token& tok) {
			wpp::dbg("(lexer) lex_comment");

			auto& ptr = lex.ptr;

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
				wpp::error(lex.position(), lex.env, "unterminated comment.");

			// Update view pointer so when the lexer continues, the token starts
			// at the right location.
			vptr = ptr;
		}


		void lex_single_comment(wpp::Lexer& lex, wpp::Token& tok) {
			wpp::dbg("(lexer) lex_single_comment");

			auto& [view, type] = tok;
			auto& [vptr, vlen] = view;

			lex.next();
			while (lex.next() != '\n');

			vptr = lex.ptr;
		}


		void lex_smart(wpp::Lexer& lex, wpp::Token& tok) {
			wpp::dbg("(lexer) lex_smart");

			// Skip 'r', 'p' or 'c'.
			++lex.ptr;
			tok.type = TOKEN_SMART;

			// Get user delimiter. 'r#"'
			//                       ^
			const char* user_delim = lex.ptr;
			++lex.ptr;

			// Make sure there's a quote here, if there isn't, we've made
			// a mistake and this is actually an identifier.
			// We call lex_identifier to perform the correct action.
			if (not wpp::in_group(lex.ptr, '\'', '"') or wpp::is_whitespace(user_delim)) {
				lex.ptr = tok.view.ptr; // Reset pointer to where it was before this function.
				lex_identifier(lex, tok);
			}
		}


		void lex_literal(wpp::token_type_t type, bool(*predicate)(const char*), wpp::Lexer& lex, wpp::Token& tok) {
			wpp::dbg("(lexer) lex_literal");

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
			wpp::dbg("(lexer) lex_simple");

			lex.next(n);
			tok.type = type;
		}


		void lex_whitespace(wpp::Lexer& lex, wpp::Token& tok) {
			wpp::dbg("(lexer) lex_whitespace");

			// Consume as much whitespace as we can.
			do {
				lex.next();
			} while (wpp::is_whitespace(lex.ptr));

			// Update view pointer so when the lexer continues, the token starts
			// at the right location.
			tok.view.ptr = lex.ptr;
		}


		void lex_identifier(wpp::Lexer& lex, wpp::Token& tok) {
			wpp::dbg("(lexer) lex_identifier");

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
			else if (view == "var")       type = TOKEN_VAR;
		}


		void lex_string_escape(wpp::Lexer& lex, wpp::Token& tok) {
			wpp::dbg("(lexer) lex_string_escape");

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
				tok.type = TOKEN_ESCAPE_HEX;

				view.ptr = ptr;

				// Get first and second nibble.
				const char* first_nibble = ptr++;
				const char* second_nibble = ptr++;

				// Check if nibbles are valid digits.
				if (not wpp::is_hex(first_nibble) or not wpp::is_hex(second_nibble))
					wpp::error(lex.position(), lex.env, "invalid character in hex escape.");
			}

			// Bin escape \b00001111.
			else if (*ptr == 'b') {
				lex.next();
				type = TOKEN_ESCAPE_BIN;

				vptr = ptr;

				// Consume 8 characters, check if each one is a valid digit.
				for (; ptr != vptr + 8; lex.next()) {
					if (not wpp::is_bin(ptr))
						wpp::error(lex.position(), lex.env, "invalid character in bin escape.");
				}
			}

			// Set view length to the number of consumed characters.
			vlen = ptr - vptr;
		}


		void lex_string_other(wpp::Lexer& lex, wpp::Token& tok) {
			wpp::dbg("(lexer) lex_string_other");

			tok.type = TOKEN_STRING;

			// Consume all characters except quotes, escapes and EOF.
			while (not wpp::in_group(lex.ptr, '\\', '"', '\'', '\0'))
				++lex.ptr;

			// Set view length equal to the number of consumed characters.
			tok.view.length = lex.ptr - tok.view.ptr;
		}


		void lex_mode_string(wpp::Lexer& lex, wpp::Token& tok) {
			wpp::dbg("(lexer) lex_mode_string");

			if (*lex.ptr == '\\')
				lex_string_escape(lex, tok);

			else
				lex_string_other(lex, tok);
		}


		void lex_mode_normal(wpp::Lexer& lex, wpp::Token& tok) {
			wpp::dbg("(lexer) lex_mode_normal");

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

			else if (*lex.ptr == '=')
				lex_simple(TOKEN_EQUAL, 1, lex, tok);

			else if (*lex.ptr == '!')
				lex_simple(TOKEN_EXCLAIM, 1, lex, tok);

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
