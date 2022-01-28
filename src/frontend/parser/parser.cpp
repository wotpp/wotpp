#include <string>
#include <vector>
#include <limits>
#include <algorithm>

#include <misc/constants.hpp>
#include <misc/fwddecl.hpp>
#include <misc/util/util.hpp>
#include <frontend/char.hpp>
#include <frontend/parser/parser.hpp>


namespace {
	inline bool peek_is_intrinsic_expr(const wpp::Token& tok) {
		return
			tok == wpp::TOKEN_INTRINSIC_RUN or
			tok == wpp::TOKEN_INTRINSIC_FILE or
			tok == wpp::TOKEN_INTRINSIC_PIPE or
			tok == wpp::TOKEN_INTRINSIC_USE
		;
	}

	inline bool peek_is_intrinsic_stmt(const wpp::Token& tok) {
		return
			tok == wpp::TOKEN_INTRINSIC_ERROR or
			tok == wpp::TOKEN_INTRINSIC_LOG or
			tok == wpp::TOKEN_INTRINSIC_ASSERT
		;
	}

	inline bool peek_is_intrinsic(const wpp::Token& tok) {
		return
			peek_is_intrinsic_expr(tok) or
			peek_is_intrinsic_stmt(tok)
		;
	}

	inline bool peek_is_smart_string(const wpp::Token& tok) {
		return
			tok == wpp::TOKEN_CODESTR or
			tok == wpp::TOKEN_RAWSTR or
			tok == wpp::TOKEN_PARASTR
		;
	}

	// Check if the token is a string.
	inline bool peek_is_string(const wpp::Token& tok) {
		return
			tok == wpp::TOKEN_DOUBLEQUOTE or
			tok == wpp::TOKEN_QUOTE or

			tok == wpp::TOKEN_STRINGIFY or

			tok == wpp::TOKEN_HEX or
			tok == wpp::TOKEN_BIN or

			peek_is_smart_string(tok)
		;
	}

	inline bool peek_is_reserved_name(const wpp::Token& tok) {
		return
			tok == wpp::TOKEN_LET or
			tok == wpp::TOKEN_DROP or
			tok == wpp::TOKEN_MATCH or
			tok == wpp::TOKEN_POP or

			peek_is_intrinsic_expr(tok) or
			peek_is_intrinsic_stmt(tok)
		;
	}

	// Check if the token is an expression.
	inline bool peek_is_expr(const wpp::Token& tok) {
		return
			tok == wpp::TOKEN_POP or
			tok == wpp::TOKEN_MATCH or
			tok == wpp::TOKEN_EVAL or
			tok == wpp::TOKEN_NEW or
			tok == wpp::TOKEN_LBRACE or

			tok == wpp::TOKEN_IDENTIFIER or

			peek_is_string(tok) or
			peek_is_intrinsic_expr(tok)
		;
	}

	// Check if the token is a statement.
	inline bool peek_is_stmt(const wpp::Token& tok) {
		return
			tok == wpp::TOKEN_LET or
			tok == wpp::TOKEN_DROP or

			peek_is_expr(tok) or
			peek_is_intrinsic_stmt(tok)
		;
	}

	inline bool peek_is_escape(const wpp::Token& tok) {
		return
			tok == wpp::TOKEN_ESCAPE_BACKSLASH or
			tok == wpp::TOKEN_ESCAPE_CARRIAGERETURN or
			tok == wpp::TOKEN_ESCAPE_NEWLINE or
			tok == wpp::TOKEN_ESCAPE_TAB or
			tok == wpp::TOKEN_ESCAPE_BIN or
			tok == wpp::TOKEN_ESCAPE_HEX or
			tok == wpp::TOKEN_ESCAPE_DOUBLEQUOTE or
			tok == wpp::TOKEN_ESCAPE_QUOTE
		;
	}
}


namespace wpp { namespace {
	// Forward declarations.
	wpp::node_t normal_string(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t stringify_string(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t hex_string(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t bin_string(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);

	wpp::node_t raw_string(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t para_string(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t code_string(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);

	wpp::node_t expression(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t fninvoke(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t match(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t block(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t codeify(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t string(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t nnew(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);

	wpp::node_t statement(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t drop(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t let(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
	wpp::node_t pop(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);

	wpp::node_t intrinsic(wpp::node_t, wpp::Lexer&, wpp::AST&, wpp::ASTMeta&, wpp::Env&);
}}


// Utils
namespace wpp { namespace {
	// Consume tokens comprising a string. Handles escape chars.
	std::string handle_escapes(const wpp::Token& part) {
		DBG();

		std::string str;

		// handle escape sequences.
		if (part == TOKEN_ESCAPE_DOUBLEQUOTE)
			str = "\"";

		else if (part == TOKEN_ESCAPE_QUOTE)
			str = "'";

		else if (part == TOKEN_ESCAPE_BACKSLASH)
			str = "\\";

		else if (part == TOKEN_ESCAPE_NEWLINE)
			str = "\n";

		else if (part == TOKEN_ESCAPE_TAB)
			str = "\t";

		else if (part == TOKEN_ESCAPE_CARRIAGERETURN)
			str = "\r";

		else if (part == TOKEN_ESCAPE_HEX) {
			// Get first and second nibble.
			uint8_t first_nibble = *part.view.ptr;
			uint8_t second_nibble = *(part.view.ptr + 1);

			// Shift the first nibble to the left by 4 and then OR it with
			// the second nibble so the first nibble is the first 4 bits
			// and the second nibble is the last 4 bits.
			str = static_cast<uint8_t>(
				wpp::hex_to_int_digit(first_nibble) << 4 |
				wpp::hex_to_int_digit(second_nibble)
			);
		}

		else if (part == TOKEN_ESCAPE_BIN) {
			const auto& [ptr, len] = part.view;
			uint8_t value = 0;

			// Shift value by 1 each loop and OR 0/1 depending on
			// the current char.
			for (size_t i = 0; i < len; ++i) {
				value <<= 1;
				value |= ptr[i] - '0';
			}

			str = value;
		}

		return str;
	}
}}


// Parser
namespace wpp { namespace {
	wpp::node_t normal_string(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		const wpp::node_t node = tree.add<String>();
		meta.emplace_back(lex.position(), parent);

		auto& str = tree.get<String>(node).value;

		const auto delim = lex.advance(wpp::lexer_modes::string); // Store delimeter.

		// Consume tokens until we reach `delim` or EOF.
		while (lex.peek(wpp::lexer_modes::string) != delim) {
			if (lex.peek(wpp::lexer_modes::string) == TOKEN_EOF)
				wpp::error(report_modes::syntax, node, env, "unterminated string", "reached EOF while parsing string literal that begins here");

			// Parse escape characters and append "parts" of the string to `str`.
			if (peek_is_escape(lex.peek(wpp::lexer_modes::string)))
				str += wpp::handle_escapes(lex.advance(wpp::lexer_modes::string));

			else
				str += lex.advance(wpp::lexer_modes::string).str();
		}

		lex.advance(); // Skip terminating quote.

		return node;
	}


	wpp::node_t stringify(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		const wpp::node_t node = tree.add<String>();
		meta.emplace_back(lex.position(), parent);

		tree.get<String>(node).value = lex.advance().str();

		return node;
	}


	wpp::node_t raw_string(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		const wpp::node_t node = tree.add<String>();
		meta.emplace_back(lex.position(), parent);
		auto& str = tree.get<String>(node).value;

		const auto delim = lex.advance().view.at(1);  // User defined delimiter.
		const auto quote = lex.advance(wpp::lexer_modes::string_raw); // ' or "

		while (true) {
			if (lex.peek(wpp::lexer_modes::string_raw) == TOKEN_EOF)
				wpp::error(report_modes::syntax, node, env, "unterminated string", "reached EOF while parsing raw string literal that begins here");

			// If we encounter ' or ", we check one character ahead to see
			// if it matches the user defined delimiter, it if does,
			// we erase the last quote character and break.
			else if (lex.peek(wpp::lexer_modes::string_raw) == quote) {
				// Store this quote, it may not actually be a part
				// of the string terminator. We append it to the string
				// if the next `if` block tests false.
				const auto tmp = lex.advance(wpp::lexer_modes::string_raw);

				if (lex.peek(wpp::lexer_modes::chr).view == delim) {
					lex.advance(wpp::lexer_modes::chr); // Skip user delimiter.
					break;  // Exit the loop, string is fully consumed.
				}

				str += tmp.str();
			}

			// If not EOF or '/", consume.
			else {
				str += lex.advance(wpp::lexer_modes::string_raw).str();
			}
		}

		return node;
	}


	wpp::node_t para_string(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		const wpp::node_t node = tree.add<String>();
		meta.emplace_back(lex.position(), parent);
		auto& str = tree.get<String>(node).value;

		const auto delim = lex.advance(wpp::lexer_modes::string_para).view.at(1);  // User defined delimiter.
		const auto quote = lex.advance(wpp::lexer_modes::string_para); // ' or "


		struct Part {
			std::string str;
			bool is_whitespace;

			Part(const std::string& str_, bool is_whitespace_):
				str(str_), is_whitespace(is_whitespace_) {}
		};

		std::vector<Part> chunks;


		while (wpp::eq_any(lex.peek(wpp::lexer_modes::string_para), TOKEN_WHITESPACE, TOKEN_WHITESPACE_NEWLINE))
			lex.advance(wpp::lexer_modes::string_para);


		while (true) {
			if (lex.peek(wpp::lexer_modes::string_para) == TOKEN_EOF)
				wpp::error(report_modes::syntax, node, env, "unterminated string",
					"reached EOF while parsing paragraph string literal that begins here"
				);

			// If we encounter ' or ", we check one character ahead to see
			// if it matches the user defined delimiter, it if does,
			// we erase the last quote character and break.
			else if (lex.peek(wpp::lexer_modes::string_para) == quote) {
				// Store this quote, it may not actually be a part
				// of the string terminator. We append it to the string
				// if the next `if` block tests false.
				const auto tmp = lex.advance(wpp::lexer_modes::string_para);

				if (lex.peek(wpp::lexer_modes::chr).view == delim) {
					lex.advance(wpp::lexer_modes::chr); // Skip user delimiter.
					break;  // Exit the loop, string is fully consumed.
				}

				// Quote was not a part of the string terminator so we append it.
				chunks.emplace_back(tmp.str(), false);
			}

			// If not EOF or '/", consume.
			else {
				const auto token = lex.advance(wpp::lexer_modes::string_para);

				// Collapse pairs of newlines into a single newline and strip any loner newlines.
				if (token == TOKEN_WHITESPACE_NEWLINE) {
					if (not chunks.empty() and chunks.back().is_whitespace)
						chunks.pop_back();

					chunks.emplace_back(token.str(), true);

					// If this newline has whitespace after it, we have to check
					// how much whitespace there is to track indentation level.
					if (lex.peek(wpp::lexer_modes::string_para) == TOKEN_WHITESPACE)
						lex.advance(wpp::lexer_modes::string_para);
				}

				// Collapse repeated whitespace of the same type.
				else if (token == TOKEN_WHITESPACE)
					chunks.emplace_back(wpp::collapse_repeated(token.str()), true);

				// Handle escape sequences.
				else if (peek_is_escape(token))
					chunks.emplace_back(wpp::handle_escapes(token), false);

				// Otherwise just append the textual parts of the string.
				else
					chunks.emplace_back(token.str(), false);
			}
		}


		// Trim trailing whitespace.
		if (not chunks.empty()) {
			auto it = chunks.rbegin();
			while (it->is_whitespace)
				++it;

			chunks.erase(it.base(), chunks.end());

			// Join chunks.
			for (const auto& [chunk, is_whitespace]: chunks)
				str += chunk;
		}

		return node;
	}


	wpp::node_t code_string(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		const wpp::node_t node = tree.add<String>();
		meta.emplace_back(lex.position(), parent);
		auto& str = tree.get<String>(node).value;

		const auto delim = lex.advance(wpp::lexer_modes::string_code).view.at(1);  // User defined delimiter.
		const auto quote = lex.advance(wpp::lexer_modes::string_code); // ' or "

		enum {
			KIND_LEADING,
			KIND_NEWLINE,
			KIND_WHITESPACE,
			KIND_OTHER,
		};

		struct Part {
			std::string str;
			uint8_t kind;

			Part(const std::string& str_, uint8_t kind_):
				str(str_), kind(kind_) {}
		};

		std::vector<Part> chunks;


		// Check if the first token is whitespace and then check if its followed
		// by text.
		// If it is followed by text, this whitespace is leading.
		if (lex.peek(wpp::lexer_modes::string_code) == TOKEN_WHITESPACE) {
			const auto tmp = lex.advance(wpp::lexer_modes::string_code).str();

			if (not wpp::eq_any(lex.peek(wpp::lexer_modes::string_code), TOKEN_WHITESPACE, TOKEN_WHITESPACE_NEWLINE))
				chunks.emplace_back(tmp, KIND_LEADING);

			else
				chunks.emplace_back(tmp, KIND_WHITESPACE);
		}


		while (true) {
			if (lex.peek(wpp::lexer_modes::string_code) == TOKEN_EOF)
				wpp::error(report_modes::syntax, node, env, "unterminated string", "reached EOF while parsing paragraph string literal that begins here");

			// If we encounter ' or ", we check one character ahead to see
			// if it matches the user defined delimiter, it if does,
			// we erase the last quote character and break.
			else if (lex.peek(wpp::lexer_modes::string_code) == quote) {
				// Store this quote, it may not actually be a part
				// of the string terminator. We append it to the string
				// if the next `if` block tests false.
				const auto tmp = lex.advance(wpp::lexer_modes::string_code);

				if (lex.peek(wpp::lexer_modes::chr).view == delim) {
					lex.advance(wpp::lexer_modes::chr); // Skip user delimiter.
					break;  // Exit the loop, string is fully consumed.
				}

				// Quote was not a part of the string terminator so we append it.
				chunks.emplace_back(tmp.str(), KIND_OTHER);
			}

			// If not EOF or '/", consume.
			else {
				const auto token = lex.advance(wpp::lexer_modes::string_code);

				// Check for newline followed by whitespace.
				if (token == TOKEN_WHITESPACE_NEWLINE) {
					chunks.emplace_back(token.str(), KIND_NEWLINE);

					// If this newline has whitespace after it, we have to check
					// how much whitespace there is to track indentation level.
					if (lex.peek(wpp::lexer_modes::string_code) == TOKEN_WHITESPACE)
						chunks.emplace_back(lex.advance(wpp::lexer_modes::string_code).str(), KIND_LEADING);

					else
						chunks.emplace_back("", KIND_LEADING);
				}

				else if (token == TOKEN_WHITESPACE)
					chunks.emplace_back(token.str(), KIND_WHITESPACE);

				// Handle escape sequences.
				else if (peek_is_escape(token))
					chunks.emplace_back(wpp::handle_escapes(token), KIND_OTHER);

				// Otherwise just append the textual parts of the string.
				else
					chunks.emplace_back(token.str(), KIND_OTHER);
			}
		}


		// Trim leading whitespace.
		for (auto it = chunks.begin(); it != chunks.end();) {
			// If we find text, break. The leading whitespace has been removed.
			if (it->kind == KIND_OTHER)
				break;

			// If we find a newline, erase from the beginning of
			// the vector to the current iterator.
			else if (it->kind == KIND_NEWLINE) {
				// We use `i + 1` here to erase this newline also.
				chunks.erase(chunks.begin(), it + 1);

				it = chunks.begin(); // Reset from beginning and search.
				continue;
			}

			++it; // We increment here so that we avoid skipping the first element on a "reset".
		}


		// Trim trailing whitespace.
		// Loop from back of vector to beginning.
		for (auto it = chunks.rbegin(); it != chunks.rend(); ++it) {
			// If we find text, erase from the back of the
			// vector to the current position of the iterator.
			if (it->kind == KIND_OTHER) {
				chunks.erase(it.base(), chunks.end());
				break;
			}
		}


		// Discover common leading whitespace amount.
		size_t common_leading_whitespace = std::numeric_limits<size_t>::max();

		for (const auto& [chunk, kind]: chunks) {
			if (kind == KIND_LEADING and chunk.size() < common_leading_whitespace)
				common_leading_whitespace = chunk.size();
		}


		// Join chunks.
		for (auto& [chunk, kind]: chunks) {
			// If this chunk is leading whitespace, strip up to `common_leading_whitespace` from the front.
			if (kind == KIND_LEADING)
				chunk.erase(chunk.begin(), chunk.begin() + common_leading_whitespace);

			str += chunk;
		}


		return node;
	}


	wpp::node_t hex_string(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env&) {
		DBG();

		const auto& [ptr, len] = lex.advance().view;

		const wpp::node_t node = tree.add<String>();
		meta.emplace_back(lex.position(), parent);
		auto& str = tree.get<String>(node).value;

		size_t counter = 0; // index into string, doesnt count `_`.

		for (size_t i = len; i > 0; i--) {
			const char c = ptr[i - 1];

			// Skip underscores.
			if (c == '_')
				continue;

			// Odd index, shift digit by 4 and or it with last character.
			if (counter & 1)
				str.back() |= wpp::hex_to_int_digit(c) << 4;

			// Even index, push back digit.
			else
				str.push_back(wpp::hex_to_int_digit(c));

			counter++;
		}

		std::reverse(str.begin(), str.end());

		return node;
	}


	wpp::node_t bin_string(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env&) {
		DBG();

		const wpp::node_t node = tree.add<String>();
		meta.emplace_back(lex.position(), parent);
		auto& str = tree.get<String>(node).value;

		const auto& [ptr, len] = lex.advance().view;

		// Reserve some space, this is kind of arbitrary.
		str.reserve(len);

		size_t counter = 0; // index into string without tracking `_`.

		for (size_t i = len; i > 0; i--) {
			const char c = ptr[i - 1];

			// Skip underscores.
			if (c == '_')
				continue;

			// We shift and or every character encounter.
			if (counter & 7)
				str.back() |= (c - '0') << (counter & 7);

			// When we get to a multiple of 8, we push back the character.
			else
				str.push_back(c - '0');

			counter++;
		}

		std::reverse(str.begin(), str.end());

		return node;
	}


	// Parse a string.
	// `"hey" 'hello' "a\nb\nc\n"`
	wpp::node_t string(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		wpp::node_t node = wpp::NODE_EMPTY;

		if (lex.peek() == TOKEN_QUOTE or lex.peek() == TOKEN_DOUBLEQUOTE)
			node = normal_string(parent, lex, tree, meta, env);

		else if (lex.peek() == TOKEN_STRINGIFY)
			node = stringify(parent, lex, tree, meta, env);

		else if (lex.peek() == TOKEN_RAWSTR)
			node = raw_string(parent, lex, tree, meta, env);

		else if (lex.peek() == TOKEN_CODESTR)
			node = code_string(parent, lex, tree, meta, env);

		else if (lex.peek() == TOKEN_PARASTR)
			node = para_string(parent, lex, tree, meta, env);

		else if (lex.peek() == TOKEN_HEX)
			node = hex_string(parent, lex, tree, meta, env);

		else if (lex.peek() == TOKEN_BIN)
			node = bin_string(parent, lex, tree, meta, env);

		return node;
	}


	// Parses a function.
	wpp::node_t let(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		// Create `Fn` node ahead of time so we can insert member data
		// directly instead of copying/moving it into a new node at the end.
		const wpp::node_t node = tree.add<Fn>();
		meta.emplace_back(lex.position(), parent);

		// Skip `let` keyword. The statement parser already checked
		// for it before calling us.
		lex.advance();


		// Make sure the next token is an identifier, if it is, set the name
		// of our `Fn` node to match.
		if (lex.peek() != TOKEN_IDENTIFIER)
			wpp::error(report_modes::syntax, lex.position(), env, "expected identifier", "expecting an identifier to follow `let`");

		tree.get<Fn>(node).identifier = lex.advance().view;


		// Variable definition
		if (peek_is_expr(lex.peek())) {
			const wpp::node_t expr = wpp::expression(parent, lex, tree, meta, env);
			tree.replace<Var>(node, tree.get<Fn>(node).identifier, expr);
			return node;
		}


		// Otherwise, this is a function definition.
		if (lex.peek() != TOKEN_LPAREN)
			wpp::error(report_modes::syntax, lex.position(), env, "expected `)`", "expecting `)` to open parameter list");

		lex.advance();  // Skip `(`.


		if (lex.peek() != TOKEN_RPAREN) {
			// Collect parameters.
			// Advance until we run out of identifiers.
			// While there is an identifier there is another parameter.
			while (lex.peek() == TOKEN_IDENTIFIER) {
				const auto param_view = lex.peek().view;
				auto& param_vec = tree.get<Fn>(node).parameters;

				if (std::find(param_vec.begin(), param_vec.end(), param_view) != param_vec.end())
					wpp::error(report_modes::syntax, lex.position(), env, "duplicate parameter",
						"multiple occurences of the same identifier in parameter list"
					);

				param_vec.emplace_back(param_view);
				lex.advance();

				if (lex.peek() == TOKEN_COMMA)
					lex.advance();
			}

			// Check if there's a keyword conflict.
			// We check if the next token is a reserved name and throw an error
			// if it is. The reason we don't check this in the while loop body is
			// because the loop condition checks for an identifier and so breaks
			// out if the next token is an intrinsic.
			if (peek_is_reserved_name(lex.peek()))
				wpp::error(report_modes::syntax, lex.position(), env, "invalid name",
					wpp::cat("parameter name '", lex.peek().str(), "' conflicts with keyword of the same name")
				);
		}

		// Make sure parameter list is terminated by `)`.
		if (lex.peek() != TOKEN_RPAREN)
			wpp::error(report_modes::syntax, lex.position(), env, "expected `)`",
				"expecting `)` to terminate parameter list",
				"there might be a non-identifier token in the parameter list"
			);

		lex.advance();

		// Parse the function body.
		const wpp::node_t body = expression(parent, lex, tree, meta, env);
		tree.get<Fn>(node).body = body;

		return node;
	}


	wpp::node_t codeify(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		wpp::node_t node = tree.add<Codeify>();
		meta.emplace_back(lex.position(), parent);

		lex.advance(); // Skip !.

		if (not peek_is_expr(lex.peek()))
			wpp::error(report_modes::syntax, lex.position(), env, "expected expression",
				"expecting an expression to follow `!`",
				"insert an expression after `!`"
			);

		const wpp::node_t expr = wpp::expression(parent, lex, tree, meta, env);
		tree.get<Codeify>(node).expr = expr;

		return node;
	}


	wpp::node_t drop(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		const wpp::node_t node = tree.add<Drop>();
		meta.emplace_back(lex.position(), parent);

		lex.advance(); // Skip `drop`.


		if (lex.peek() != TOKEN_IDENTIFIER)
			wpp::error(report_modes::syntax, node, env, "expected identifier", "expecting identifier to follow `drop`");


		const auto identifier = lex.advance().view;
		tree.get<Drop>(node).identifier = identifier;


		lex.advance();  // Skip `(`.


		// Consume list of identifiers.
		while (lex.peek() == TOKEN_IDENTIFIER) {
			tree.get<Drop>(node).n_args++;
			lex.advance();

			if (lex.peek() == TOKEN_COMMA)
				lex.advance();
		}

		if (lex.peek() != TOKEN_RPAREN)
			wpp::error(report_modes::syntax, lex.position(), env, "expected `)`",
				"expecting `)` to follow argument list",
				"there might be a non-identifier token in the argument list"
			);

		lex.advance();

		return node;
	}


	wpp::node_t intrinsic(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		meta.emplace_back(lex.position(), parent);
		const wpp::Token tok = lex.advance();

		wpp::node_t node = NODE_EMPTY;

		const wpp::node_t expr = wpp::expression(parent, lex, tree, meta, env);

		if (tok == TOKEN_INTRINSIC_USE)
			node = tree.add<IntrinsicUse>(expr);

		else if (tok == TOKEN_INTRINSIC_RUN)
			node = tree.add<IntrinsicRun>(expr);

		else if (tok == TOKEN_INTRINSIC_FILE)
			node = tree.add<IntrinsicFile>(expr);

		else if (tok == TOKEN_INTRINSIC_ERROR)
			node = tree.add<IntrinsicError>(expr);

		else if (tok == TOKEN_INTRINSIC_LOG)
			node = tree.add<IntrinsicLog>(expr);

		else if (tok == TOKEN_INTRINSIC_ASSERT) {
			const wpp::node_t expr2 = wpp::expression(parent, lex, tree, meta, env);
			node = tree.add<IntrinsicAssert>(expr, expr2);
		}

		else if (tok == TOKEN_INTRINSIC_PIPE) {
			const wpp::node_t expr2 = wpp::expression(parent, lex, tree, meta, env);
			node = tree.add<IntrinsicPipe>(expr, expr2);
		}

		return node;
	}


	wpp::node_t pop(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		const wpp::node_t node = tree.add<Pop>();
		meta.emplace_back(lex.position(), parent);

		lex.advance(); // skip `pop`.


		if (lex.peek() != TOKEN_IDENTIFIER)
			wpp::error(report_modes::syntax, lex.position(), env, "expected identifier", "expecting identifier to follow `pop`");

		tree.get<Pop>(node).identifier = lex.advance().view;


		if (lex.peek() != TOKEN_LPAREN)
			wpp::error(report_modes::syntax, lex.position(), env, "expected `(`", "expecting `(` to open argument list");

		lex.advance();  // Skip `(`.


		if (lex.peek() != TOKEN_RPAREN) {
			// While there is an expression there is another parameter.
			while (peek_is_expr(lex.peek())) {
				wpp::node_t expr = expression(parent, lex, tree, meta, env);
				tree.get<Pop>(node).arguments.emplace_back(expr);

				if (lex.peek() == TOKEN_COMMA)
					lex.advance();
			}
		}


		if (lex.peek() != TOKEN_STAR)
			wpp::error(report_modes::syntax, node, env, "no substitute argument",
				"the function template must specify an argument with which to substitute the popped value",
				"insert `*` somewhere in the argument list to specify that the popped value will be passed to that index"
			);


		while (lex.peek() == TOKEN_STAR) {
			tree.get<Pop>(node).n_popped_args++;
			lex.advance();

			if (lex.peek() == TOKEN_COMMA)
				lex.advance();
		}


		// Make sure argument list is terminated by `)`.
		if (lex.peek() != TOKEN_RPAREN)
			wpp::error(report_modes::syntax, lex.position(), env, "expected `)`",
				"expecting `)` to terminate argument list",
				"`*` must come at the end of the argument list"
			);

		lex.advance();

		return node;
	}


	// Parse a function call.
	wpp::node_t fninvoke(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		wpp::node_t node = tree.add<FnInvoke>();
		meta.emplace_back(lex.position(), parent);

		tree.get<FnInvoke>(node).identifier = lex.advance().view;

		// Optional arguments.
		if (lex.peek() != TOKEN_LPAREN) {
			tree.replace<VarRef>(node, tree.get<FnInvoke>(node).identifier);
			return node;
		}


		lex.advance();  // Skip `(`.

		// While there is an expression there is another parameter.
		while (peek_is_expr(lex.peek())) {
			// Parse expr.
			wpp::node_t expr = expression(parent, lex, tree, meta, env);
			tree.get<FnInvoke>(node).arguments.emplace_back(expr);

			if (lex.peek() == TOKEN_COMMA)
				lex.advance();
		}

		// Make sure parameter list is terminated by `)`.
		if (lex.advance() != TOKEN_RPAREN)
			wpp::error(report_modes::syntax, lex.position(), env, "expected `)`",
				"expecting `)` to follow argument list",
				"there might be a non-identifier token in the argument list"
			);

		return node;
	}


	// Parse a block.
	wpp::node_t block(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		const wpp::node_t node = tree.add<Block>();
		meta.emplace_back(lex.position(), parent);

		lex.advance(); // Skip '{'.

		if (lex.peek() == TOKEN_RBRACE)
			wpp::error(report_modes::syntax, lex.position(), env, "expected expression",
				"expecting a trailing expression at the end of block"
			);


		// Check for statement, otherwise we parse a single expression.
		// last_is_expr is used to check if the last statement holds
		// an expression, if it does we need to back up after parsing
		// the last statement to consider it as the trailing expression
		// of the block.
		bool last_is_expr = false;

		if (peek_is_stmt(lex.peek())) {
			// Consume statements.
			do {
				last_is_expr = peek_is_expr(lex.peek());

				const wpp::node_t stmt = statement(parent, lex, tree, meta, env);
				tree.get<Block>(node).statements.emplace_back(stmt);
			} while (peek_is_stmt(lex.peek()));
		}


		// If the next token is not an expression and the last statement
		// was an expression then we can pop the last statement and use
		// it as our trailing expression.
		if (not peek_is_expr(lex.peek()) and last_is_expr) {
			tree.get<Block>(node).expr = tree.get<Block>(node).statements.back();
			tree.get<Block>(node).statements.pop_back();
		}

		else {
			wpp::error(report_modes::syntax, node, env, "expected expression",
				"expecting a trailing expression at the end of block that begins here"
			);
		}


		if (lex.peek() == TOKEN_ARROW)
			wpp::error(report_modes::syntax, lex.position(), env, "unexpected `->`",
				"found `->` inside a block expression",
				"did you forget the test expression for match?"
			);

		// Expect '}'.
		if (lex.peek() != TOKEN_RBRACE) {
			wpp::error(report_modes::syntax, node, env, "expected `}`",
				"expecting `}` to terminate block expression that begins here"
			);
		}

		lex.advance(); // Skip '}'.

		return node;
	}


	wpp::node_t match(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		const wpp::node_t node = tree.add<Match>();
		meta.emplace_back(lex.position(), parent);

		lex.advance(); // Skip `match`.


		// Check for test expression.
		if (not peek_is_expr(lex.peek()))
			wpp::error(report_modes::syntax, lex.position(), env, "expected expression",
				"expecting an expression to follow `match`",
				"insert a test expression for `match` to match on"
			);


		const auto expr = wpp::expression(parent, lex, tree, meta, env); // Consume test expression.
		tree.get<Match>(node).expr = expr;


		if (lex.peek() != TOKEN_LBRACE)
			wpp::error(report_modes::syntax, lex.position(), env, "expected `{`", "expecting `{` to begin match expression body");

		lex.advance();


		// Collect all arms of the match.
		while (peek_is_expr(lex.peek())) {
			const auto arm = wpp::expression(parent, lex, tree, meta, env);

			if (lex.peek() != TOKEN_ARROW)
				wpp::error(report_modes::syntax, lex.position(), env, "expected `->`",
					"expecting `->` to denote right hand side of match arm"
				);

			lex.advance();

			if (not peek_is_expr(lex.peek()))
				wpp::error(report_modes::syntax, lex.position(), env, "expected expression", "expecting an expression after `->`");

			const auto hand = wpp::expression(parent, lex, tree, meta, env);

			tree.get<Match>(node).cases.emplace_back(std::pair{ arm, hand });
		}


		// Optional default case.
		if (lex.peek() == TOKEN_STAR) {
			lex.advance();

			if (lex.peek() != TOKEN_ARROW)
				wpp::error(report_modes::syntax, lex.position(), env, "expected `->`",
					"expecting `->` to denote right hand side of match arm"
				);

			lex.advance();

			if (not peek_is_expr(lex.peek()))
				wpp::error(report_modes::syntax, lex.position(), env, "expected expression", "expecting an expression after `->`");

			const auto default_case = wpp::expression(parent, lex, tree, meta, env);
			tree.get<Match>(node).default_case = default_case;
		}

		else {
			tree.get<Match>(node).default_case = wpp::NODE_EMPTY;
		}


		if (lex.peek() != TOKEN_RBRACE)
			wpp::error(report_modes::syntax, node, env, "expected `}`",
				"expecting `}` to terminate match expression that begins here"
			);

		lex.advance();

		return node;
	}


	wpp::node_t nnew(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		const wpp::node_t node = tree.add<New>();
		meta.emplace_back(lex.position(), parent);

		lex.advance(); // Skip `new`.


		if (not peek_is_expr(lex.peek()))
			wpp::error(report_modes::syntax, lex.position(), env, "expected expression", "expecting an expression to follow `new`");

		const wpp::node_t expr = wpp::expression(parent, lex, tree, meta, env);
		tree.get<New>(node).expr = expr;

		return node;
	}


	// Parse an expression.
	wpp::node_t expression(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		env.rec_depth++;

		if (
			env.flags & wpp::WARN_DEEP_EXPRESSION and
			env.rec_depth >= 64 and
			not wpp::is_previously_seen_warning(WARN_DEEP_EXPRESSION, parent, env)
		)
			wpp::warning(report_modes::syntax, parent, env, "deep expression",
				"the recursion depth has grown to >= 64",
				"this may indicate deeply nested expressions"
			);


		// We use lhs to store the resulting expression
		// from the following cases and if the next token
		// is concatenation, we make a new Concat node using
		// lhs as the left hand side of the node and then continue on to
		// parse another expression for the right hand side.
		wpp::node_t lhs;

		const auto lookahead = lex.peek();

		if (lookahead == TOKEN_IDENTIFIER)
			lhs = wpp::fninvoke(parent, lex, tree, meta, env);

		else if (peek_is_string(lookahead))
			lhs = wpp::string(parent, lex, tree, meta, env);

		else if (peek_is_intrinsic_expr(lookahead))
			lhs = wpp::intrinsic(parent, lex, tree, meta, env);

		else if (lookahead == TOKEN_LBRACE)
			lhs = wpp::block(parent, lex, tree, meta, env);

		else if (lookahead == TOKEN_EVAL)
			lhs = wpp::codeify(parent, lex, tree, meta, env);

		else if (lookahead == TOKEN_MATCH)
			lhs = wpp::match(parent, lex, tree, meta, env);

		else if (lookahead == TOKEN_POP)
			lhs = wpp::pop(parent, lex, tree, meta, env);

		else if (lookahead == TOKEN_NEW)
			lhs = wpp::nnew(parent, lex, tree, meta, env);

		else
			wpp::error(report_modes::syntax, lex.position(), env, "expected expression", "expecting an expression to appear here");


		if (lex.peek() == TOKEN_CAT) {
			const wpp::node_t node = tree.add<Concat>();
			meta.emplace_back(lex.position(), parent);

			lex.advance(); // Skip `..`.
			tree.get<Concat>(node).lhs = lhs;

			const node_t rhs = expression(parent, lex, tree, meta, env);
			tree.get<Concat>(node).rhs = rhs;

			lhs = node;
		}

		else if (lex.peek() == TOKEN_LBRACKET) {
			const wpp::node_t node = tree.add<Slice>();
			meta.emplace_back(lex.position(), parent);

			tree.get<Slice>(node).expr = lhs;

			lex.advance(); // Skip `[`.


			// Index/Start
			if (lex.peek(lexer_modes::slice) == TOKEN_INT) {
				tree.get<Slice>(node).start = lex.advance(lexer_modes::slice).view;

				// Check for `:`. If we find one, the first integer literal was
				// actually the start index of a slice and not an index.
				if (lex.peek(lexer_modes::slice) == TOKEN_COLON) {
					tree.get<Slice>(node).set = Slice::SLICE_START;
					lex.advance(lexer_modes::slice); // Skip `:`.

					// Stop
					if (lex.peek(lexer_modes::slice) == TOKEN_INT) {
						tree.get<Slice>(node).stop = lex.advance(lexer_modes::slice).view;
						tree.get<Slice>(node).set |= Slice::SLICE_STOP;
					}
				}

				else
					tree.get<Slice>(node).set = Slice::SLICE_INDEX;
			}

			else if (lex.peek(lexer_modes::slice) == TOKEN_COLON) {
				tree.get<Slice>(node).set = Slice::SLICE_STOP;
				lex.advance(lexer_modes::slice); // Skip `:`.

				if (lex.peek(lexer_modes::slice) != TOKEN_INT)
					wpp::error(report_modes::syntax, lex.position(), env, "expected integer literal",
						"expecting an integer literal for stop index"
					);

				tree.get<Slice>(node).stop = lex.advance(lexer_modes::slice).view;
			}


			if (tree.get<Slice>(node).set == 0)
				wpp::error(report_modes::syntax, lex.position(), env, "empty slice", "expecting slice indices or range");


			if (lex.peek(lexer_modes::slice) != TOKEN_RBRACKET)
				wpp::error(report_modes::syntax, lex.position(), env, "expected `]`", "expecting `]` to terminate string slice");


			lex.advance(lexer_modes::slice);

			lhs = node;
		}

		env.rec_depth--;

		return lhs;
	}


	// Parse a statement.
	wpp::node_t statement(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		env.rec_depth++;

		if (
			env.flags & wpp::WARN_DEEP_EXPRESSION and
			env.rec_depth >= wpp::MAX_EXPR_DEPTH and
			not wpp::is_previously_seen_warning(WARN_DEEP_EXPRESSION, parent, env)
		)
			wpp::warning(report_modes::syntax, parent, env, "deep expression",
				wpp::cat("the nested expression depth has grown to >= ", wpp::MAX_EXPR_DEPTH),
				"this may indicate deeply nested expressions"
			);


		const auto lookahead = lex.peek();
		wpp::node_t node;

		if (lookahead == TOKEN_LET)
			node = wpp::let(parent, lex, tree, meta, env);

		else if (lookahead == TOKEN_DROP)
			node = wpp::drop(parent, lex, tree, meta, env);

		else if (peek_is_intrinsic_stmt(lookahead))
			node = wpp::intrinsic(parent, lex, tree, meta, env);

		else if (peek_is_expr(lookahead))
			node = wpp::expression(parent, lex, tree, meta, env);

		else
			wpp::error(report_modes::syntax, lex.position(), env, "expected statement", "expecting a statement to appear here");

		env.rec_depth--;

		return node;
	}
}}


namespace wpp {
	// Parse a document.
	// A document is just a series of zero or more expressions.
	wpp::node_t document(wpp::node_t parent, wpp::Lexer& lex, wpp::AST& tree, wpp::ASTMeta& meta, wpp::Env& env) {
		DBG();

		const wpp::node_t node = tree.add<Document>();
		meta.emplace_back(lex.position(), parent);

		// Consume expressions until we encounter eof or an error.
		while (lex.peek() != TOKEN_EOF) {
			try {
				const wpp::node_t stmt = statement(parent, lex, tree, meta, env);
				tree.get<Document>(node).statements.emplace_back(stmt);
			}

			catch (const wpp::Report& e) {
				env.state |= wpp::ABORT_EVALUATION;

				// Early out if this is a lexer or utf-8 validation error.
				if (env.state & wpp::ABORT_ERROR_RECOVERY)
					throw; // Propagate error.

				env.state |= wpp::ERROR_MODE_PARSE;

				lex.advance();

				wpp::Report last_report = e;

				while (lex.peek() != TOKEN_EOF) {
					try {
						wpp::statement(parent, lex, tree, meta, env);

					} catch (const wpp::Report& e) {
						// We have to check for MAX_ERRORS - 1 because this error will be reported also.
						if (env.report_count >= wpp::MAX_ERRORS - 1)
							throw;

						std::cerr << last_report.str();

						last_report = e;
						lex.advance();
					}
				}

				throw last_report;
			}
		}

		return node;
	}
}
