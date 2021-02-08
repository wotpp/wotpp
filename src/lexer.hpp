#pragma once

#ifndef WOTPP_LEXER
#define WOTPP_LEXER

#include <structures/token.hpp>
#include <structures/position.hpp>
#include <utils/char.hpp>

#include <tinge.hpp>

namespace wpp {
	namespace modes {
		enum {
			normal,
			string,
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
		TOKEN(TOKEN_ERROR) \
		TOKEN(TOKEN_EOF) \
		\
		TOKEN(TOKEN_WHITESPACE) \
		TOKEN(TOKEN_SLASH) \
		TOKEN(TOKEN_BACKSLASH) \
		TOKEN(TOKEN_CAT) \
		TOKEN(TOKEN_COMMA) \
		\
		TOKEN(TOKEN_IDENTIFIER) \
		TOKEN(TOKEN_NAMESPACE) \
		TOKEN(TOKEN_LET) \
		TOKEN(TOKEN_RUN) \
		TOKEN(TOKEN_FILE) \
		TOKEN(TOKEN_EVAL) \
		TOKEN(TOKEN_ASSERT) \
		\
		TOKEN(TOKEN_LPAREN) \
		TOKEN(TOKEN_RPAREN) \
		TOKEN(TOKEN_LBRACE) \
		TOKEN(TOKEN_RBRACE) \
		\
		TOKEN(TOKEN_ESCAPE_NEWLINE) \
		TOKEN(TOKEN_ESCAPE_TAB) \
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
		TOKEN(TOKEN_RAW) \
		TOKEN(TOKEN_PARA)



	#define TOKEN(x) x,
		enum { TOKEN_TYPES };
	#undef TOKEN

	#define TOKEN(x) #x,
		constexpr const char* to_str[] = { TOKEN_TYPES };
	#undef TOKEN

	#undef TOKEN_TYPES
}

namespace wpp {
	class Lexer {
		private:
			const char* const start = nullptr;
			const char* str = nullptr;

			wpp::Token lookahead{};
			int lookahead_mode = modes::normal;


		public:
			Lexer(const char* const str_, int mode_ = modes::normal):
				start(str_),
				str(str_),
				lookahead(),
				lookahead_mode(mode_)
			{
				advance(mode_);
			}


		public:
			const wpp::Token& peek(int mode = modes::normal) {
				if (mode != lookahead_mode) {
					str = lookahead.view.ptr; // reset pointer.
					lookahead = next_token(mode);
					lookahead_mode = mode;
				}

				return lookahead;
			}

			wpp::Token advance(int mode = modes::normal) {
				auto tok = peek(mode);
				lookahead = next_token(mode);
				return tok;
			}

			wpp::Position position() const {
				return wpp::position(start, lookahead.view.ptr);
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

					if (*str == '\0') {
						type = TOKEN_EOF;

					}

					else if (*str == '\'') { ++str; type = TOKEN_QUOTE; }
					else if (*str == '"')  { ++str; type = TOKEN_DOUBLEQUOTE; }

					else if (mode == modes::normal) {
						// comment
						if (*str == '#' and *(str + 1) == '[') {
							str += 2;

							int depth = 1;

							while (depth > 0 and *str != '\0') {
								if (*str == '#' and *(str + 1) == '[') {
									depth++;
									str += 2;
								}

								else if (*str == ']') {
									depth--;
									++str;
								}

								else {
									++str;
								}
							}

							if (*str == '\0' and depth != 0) {
								wpp::error(position(), "unterminated comment.");
							}

							// return next_token(mode);
							continue;
						}

						// raw string
						else if (*str == 'r' and (*(str + 1) == '"' or *(str + 1) == '\'')) {
							str += 2;
							type = TOKEN_RAW;
							vlen = str - vptr;
						}

						// paragraph
						else if (*str == 'p' and (*(str + 1) == '"' or *(str + 1) == '\'')) {
							str += 2;
							type = TOKEN_PARA;
							vlen = str - vptr;
						}

						// bin literal
						else if (*str == '0' and *(str + 1) == 'b') {
							str += 2;
							type = TOKEN_BIN;
							vlen = str - vptr;
						}

						// hex literal
						else if (*str == '0' and *(str + 1) == 'x') {
							str += 2;
							type = TOKEN_HEX;
							vlen = str - vptr;
						}


						// cat
						else if (*str == '.' and *(str + 1) == '.') {
							str += 2;
							type = TOKEN_CAT;
							vlen = str - vptr;
						}

						// other
						else if (*str == ',')  { ++str; type = TOKEN_COMMA; }

						else if (*str == '(')  { ++str; type = TOKEN_LPAREN; }
						else if (*str == ')')  { ++str; type = TOKEN_RPAREN; }

						else if (*str == '{')  { ++str; type = TOKEN_LBRACE; }
						else if (*str == '}')  { ++str; type = TOKEN_RBRACE; }

						// Skip whitespace.
						else if (wpp::is_whitespace(*str)) {
							do {
								++str;
							} while (wpp::is_whitespace(*str));

							// return next_token(mode);
							continue;
						}

						// Handle identifiers.
						else {
							type = TOKEN_IDENTIFIER;

							do {
								++str;
							} while (
								// make sure we don't run into a character that belongs to a token above.
								not wpp::is_whitespace(*str) and
								not wpp::in_group(*str, '(', ')', '{', '}', '#', ',', '\0', '\'', '"', '.')
							);

							vlen = str - vptr;

							if      (view == "let")       type = TOKEN_LET;
							else if (view == "namespace") type = TOKEN_NAMESPACE;
							else if (view == "run")       type = TOKEN_RUN;
							else if (view == "eval")      type = TOKEN_EVAL;
							else if (view == "file")      type = TOKEN_FILE;
							else if (view == "assert")    type = TOKEN_ASSERT;
						}

						// else {
						// 	wpp::error(position(), "unexpected character `", *str, "`(", (int)*str, ").");
						// }
					}

					else if (mode == modes::string) {
						// Escape characters, `\n`, `\t` etc...
						if (*str == '\\') {
							++str;
							type = TOKEN_BACKSLASH;

							if      (*str == '\\') { ++str; type = TOKEN_ESCAPE_BACKSLASH; }
							else if (*str == '\'') { ++str; type = TOKEN_ESCAPE_QUOTE; }
							else if (*str == '"')  { ++str; type = TOKEN_ESCAPE_DOUBLEQUOTE; }
							else if (*str == 't')  { ++str; type = TOKEN_ESCAPE_TAB; }
							else if (*str == 'n')  { ++str; type = TOKEN_ESCAPE_NEWLINE; }
							else if (*str == 'x')  { ++str; type = TOKEN_ESCAPE_HEX; }
							else if (*str == 'b')  { ++str; type = TOKEN_ESCAPE_BIN; }

							vlen = str - vptr;
						}

						else {
							type = TOKEN_STRING;

							do {
								++str;
							} while (not wpp::in_group(*str, '\\', '"', '\'', '\0'));

							vlen = str - vptr;
						}
					}

					break;
				}

				return tok;
			}
	};
}

#endif
