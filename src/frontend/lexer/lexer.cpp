#include <frontend/lexer/lexer.hpp>
#include <structures/exception.hpp>
#include <frontend/char.hpp>


namespace wpp {
	wpp::Token Lexer::next_token(int mode) {
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
}


namespace wpp {
	void lex_comment(wpp::Lexer& lex, wpp::Token& tok) {
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


	void lex_single_comment(wpp::Lexer& lex, wpp::Token& tok) {
		auto [start, ptr] = lex.get_ptrs();

		auto& [view, type] = tok;
		auto& [vptr, vlen] = view;

		lex.next();
		while (lex.next() != '\n');

		vptr = ptr;
	}


	void lex_smart(wpp::Lexer& lex, wpp::Token& tok) {
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
	}


	void lex_literal(wpp::token_type_t type, bool(*predicate)(char), wpp::Lexer& lex, wpp::Token& tok) {
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
	void lex_simple(wpp::token_type_t type, int n, wpp::Lexer& lex, wpp::Token& tok) {
		lex.next(n);
		tok.type = type;
	}


	void lex_whitespace(wpp::Lexer& lex, wpp::Token& tok) {
		// Consume as much whitespace as we can.
		do {
			lex.next();
		} while (wpp::is_whitespace(*lex.str));

		// Update view pointer so when the lexer continues, the token starts
		// at the right location.
		tok.view.ptr = lex.str;
	}


	void lex_identifier(wpp::Lexer& lex, wpp::Token& tok) {
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
		else if (view == "var")       type = TOKEN_VAR;
	}


	void lex_string_escape(wpp::Lexer& lex, wpp::Token& tok) {
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


	void lex_string_other(wpp::Lexer& lex, wpp::Token& tok) {
		tok.type = TOKEN_STRING;

		// Consume all characters except quotes, escapes and EOF.
		while (not wpp::in_group(*lex.str, '\\', '"', '\'', '\0'))
			++lex.str;

		// Set view length equal to the number of consumed characters.
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

		else if (*lex.str == '=')
			lex_simple(TOKEN_EQUAL, 1, lex, tok);

		else if (*lex.str == '!')
			lex_simple(TOKEN_EXCLAIM, 1, lex, tok);

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
