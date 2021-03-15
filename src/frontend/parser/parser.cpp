#include <string>
#include <vector>
#include <limits>
#include <algorithm>

#include <misc/fwddecl.hpp>
#include <misc/util/util.hpp>
#include <frontend/char.hpp>
#include <frontend/parser/parser.hpp>


namespace {
	inline bool peek_is_intrinsic(const wpp::Token& tok) {
		return
			tok == wpp::TOKEN_RUN or
			tok == wpp::TOKEN_EVAL or
			tok == wpp::TOKEN_FILE or
			tok == wpp::TOKEN_ASSERT or
			tok == wpp::TOKEN_PIPE or
			tok == wpp::TOKEN_ERROR or
			tok == wpp::TOKEN_SOURCE or
			tok == wpp::TOKEN_SLICE or
			tok == wpp::TOKEN_FIND or
			tok == wpp::TOKEN_LENGTH or
			tok == wpp::TOKEN_ESCAPE or
			tok == wpp::TOKEN_LOG
		;
	}

	inline bool peek_is_keyword(const wpp::Token& tok) {
		return
			tok == wpp::TOKEN_LET or
			tok == wpp::TOKEN_VAR or
			tok == wpp::TOKEN_PREFIX or
			tok == wpp::TOKEN_DROP
		;
	}

	// Check if the token is a string.
	inline bool peek_is_string(const wpp::Token& tok) {
		return
			tok == wpp::TOKEN_DOUBLEQUOTE or
			tok == wpp::TOKEN_QUOTE or

			tok == wpp::TOKEN_EXCLAIM or

			tok == wpp::TOKEN_HEX or
			tok == wpp::TOKEN_BIN or

			tok == wpp::TOKEN_SMART
		;
	}

	inline bool peek_is_reserved_name(const wpp::Token& tok) {
		return
			peek_is_intrinsic(tok) or
			peek_is_keyword(tok)
		;
	}

	// Check if the token is an expression.
	inline bool peek_is_call(const wpp::Token& tok) {
		return
			tok == wpp::TOKEN_IDENTIFIER or
			peek_is_intrinsic(tok)
		;
	}

	// Check if the token is an expression.
	inline bool peek_is_expr(const wpp::Token& tok) {
		return
			tok == wpp::TOKEN_MAP or
			tok == wpp::TOKEN_EQUAL or
			tok == wpp::TOKEN_LBRACE or
			peek_is_string(tok) or
			peek_is_call(tok)
		;
	}

	// Check if the token is a statement.
	inline bool peek_is_stmt(const wpp::Token& tok) {
		return
			peek_is_keyword(tok) or
			peek_is_expr(tok)
		;
	}
}


namespace wpp {
	namespace {
		// Consume tokens comprising a string. Handles escape chars.
		void accumulate_string(const wpp::Token&, std::string&, bool = true);

		// Forward declarations.
		void normal_string(wpp::Lexer&, std::string&, wpp::Env&);
		void stringify_string(wpp::Lexer&, std::string&, wpp::Env&);
		void smart_string(wpp::Lexer&, std::string&, wpp::Env&);
		void hex_string(wpp::Lexer&, std::string&, wpp::Env&);
		void bin_string(wpp::Lexer&, std::string&, wpp::Env&);

		void raw_string(std::string&);
		void para_string(std::string&);
		void code_string(std::string&);

		wpp::node_t expression(wpp::Lexer&, wpp::AST&, wpp::Positions&, wpp::Env&);
		wpp::node_t fninvoke(wpp::Lexer&, wpp::AST&, wpp::Positions&, wpp::Env&);
		wpp::node_t map(wpp::Lexer&, wpp::AST&, wpp::Positions&, wpp::Env&);
		wpp::node_t block(wpp::Lexer&, wpp::AST&, wpp::Positions&, wpp::Env&);
		wpp::node_t codeify(wpp::Lexer&, wpp::AST&, wpp::Positions&, wpp::Env&);
		wpp::node_t string(wpp::Lexer&, wpp::AST&, wpp::Positions&, wpp::Env&);

		wpp::node_t statement(wpp::Lexer&, wpp::AST&, wpp::Positions&, wpp::Env&);
		wpp::node_t var(wpp::Lexer&, wpp::AST&, wpp::Positions&, wpp::Env&);
		wpp::node_t drop(wpp::Lexer&, wpp::AST&, wpp::Positions&, wpp::Env&);
		wpp::node_t let(wpp::Lexer&, wpp::AST&, wpp::Positions&, wpp::Env&);
		wpp::node_t prefix(wpp::Lexer&, wpp::AST&, wpp::Positions&, wpp::Env&);
	}
}


namespace wpp {
	namespace {
		// Consume tokens comprising a string. Handles escape chars.
		void accumulate_string(const wpp::Token& part, std::string& literal, bool handle_escapes) {
			wpp::dbg("(parser) accumulate_string");

			if (not handle_escapes) {
				literal.append(part.str());
				return;
			}

			// handle escape sequences.
			if (part == TOKEN_ESCAPE_DOUBLEQUOTE)
				literal.append("\"");

			else if (part == TOKEN_ESCAPE_QUOTE)
				literal.append("'");

			else if (part == TOKEN_ESCAPE_BACKSLASH)
				literal.append("\\");

			else if (part == TOKEN_ESCAPE_NEWLINE)
				literal.append("\n");

			else if (part == TOKEN_ESCAPE_TAB)
				literal.append("\t");

			else if (part == TOKEN_ESCAPE_CARRIAGERETURN)
				literal.append("\r");

			else if (part == TOKEN_ESCAPE_HEX) {
				// Get first and second nibble.
				uint8_t first_nibble = *part.view.ptr;
				uint8_t second_nibble = *(part.view.ptr + 1);

				// Shift the first nibble to the left by 4 and then OR it with
				// the second nibble so the first nibble is the first 4 bits
				// and the second nibble is the last 4 bits.
				literal += static_cast<uint8_t>(
					wpp::hex_to_digit(first_nibble) << 4 |
					wpp::hex_to_digit(second_nibble)
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

				literal += value;
			}

			else
				literal.append(part.str());
		}




		// Parses a function.
		wpp::node_t let(wpp::Lexer& lex, wpp::AST& tree, wpp::Positions& pos, wpp::Env& env) {
			wpp::dbg("(parser) let");

			// Create `Fn` node ahead of time so we can insert member data
			// directly instead of copying/moving it into a new node at the end.
			const wpp::node_t node = tree.add<Fn>();
			pos.emplace_back(lex.position());

			// Skip `let` keyword. The statement parser already checked
			// for it before calling us.
			lex.advance();


			// Make sure the next token is an identifier, if it is, set the name
			// of our `Fn` node to match.
			if (lex.peek() != TOKEN_IDENTIFIER)
				wpp::error(pos.at(node), env, "function declaration does not have a name.");

			tree.get<Fn>(node).identifier = lex.advance().view;


			// Collect parameters.
			// Advance until we run out of identifiers.
			if (lex.peek() == TOKEN_LPAREN) {
				lex.advance();  // Skip `(`.


				// While there is an identifier there is another parameter.
				while (lex.peek() == TOKEN_IDENTIFIER) {
					// Advance the lexer and get the identifier.
					auto id = lex.advance().view;

					// Add the argument
					tree.get<Fn>(node).parameters.emplace_back(id);

					// If the next token is a comma, skip it.
					if (lex.peek() == TOKEN_COMMA)
						lex.advance(); // skip the comma

					// Otherwise it must be ')'?
					else if (lex.peek() != TOKEN_RPAREN)
						// If it's not, throw an exception.
						wpp::error(pos.at(node), env, "expecting comma to follow parameter name.");
				}

				// Check if there's an keyword conflict.
				// We check if the next token is a reserved name and throw an error
				// if it is. The reason we don't check this in the while loop body is
				// because the loop condition checks for an identifier and so breaks
				// out if the next token is an intrinsic.
				if (peek_is_reserved_name(lex.peek()))
					wpp::error(pos.at(node), env, "parameter name '", lex.advance().str(), "' conflicts with keyword.");

				// // Make sure parameter list is terminated by `)`.
				if (lex.advance() != TOKEN_RPAREN)
					wpp::error(pos.at(node), env, "expecting ')' to follow argument list.");
			}

			// Parse the function body.
			const wpp::node_t body = expression(lex, tree, pos, env);
			tree.get<Fn>(node).body = body;

			return node;
		}


		wpp::node_t var(wpp::Lexer& lex, wpp::AST& tree, wpp::Positions& pos, wpp::Env& env) {
			wpp::dbg("(parser) var");

			// Create `Var` node ahead of time so we can insert member data
			// directly instead of copying/moving it into a new node at the end.
			const wpp::node_t node = tree.add<Var>();
			pos.emplace_back(lex.position());


			// Skip `var` keyword. The statement parser already checked
			// for it before calling us.
			lex.advance();

			// Make sure the next token is an identifier, if it is, set the name
			// of our `Fn` node to match.
			if (lex.peek() != TOKEN_IDENTIFIER)
				wpp::error(pos.at(node), env, "variable declaration does not have a name.");


			tree.get<Var>(node).identifier = lex.advance().view;

			// Parse the variable body.
			const wpp::node_t body = expression(lex, tree, pos, env);
			tree.get<Var>(node).body = body;

			return node;
		}


		wpp::node_t codeify(wpp::Lexer& lex, wpp::AST& tree, wpp::Positions& pos, wpp::Env& env) {
			wpp::dbg("(parser) codeify");

			lex.advance(); // Skip =.

			wpp::node_t node = tree.add<Codeify>();
			pos.emplace_back(lex.position());

			if (not peek_is_expr(lex.peek()))
				wpp::error(pos.at(node), env, "expecting an expression to follow =.");

			const wpp::node_t expr = wpp::expression(lex, tree, pos, env);
			tree.get<Codeify>(node).expr = expr;

			return node;
		}


		wpp::node_t drop(wpp::Lexer& lex, wpp::AST& tree, wpp::Positions& pos, wpp::Env& env) {
			wpp::dbg("(parser) drop");

			lex.advance(); // Skip `drop`.

			const wpp::node_t node = tree.add<Drop>();
			pos.emplace_back(lex.position());


			const wpp::node_t call_expr = wpp::fninvoke(lex, tree, pos, env);
			tree.get<Drop>(node).func = call_expr;

			return node;
		}


		void normal_string(wpp::Lexer& lex, std::string& str, wpp::Env& env) {
			wpp::dbg("(parser) normal_string");

			const auto delim = lex.advance(wpp::lexer_modes::string); // Store delimeter.

			// Consume tokens until we reach `delim` or EOF.
			while (lex.peek(wpp::lexer_modes::string) != delim) {
				if (lex.peek(wpp::lexer_modes::string) == TOKEN_EOF)
					wpp::error(lex.position(), env, "reached EOF while parsing string.");

				// Parse escape characters and append "parts" of the string to `str`.
				accumulate_string(lex.advance(wpp::lexer_modes::string), str);
			}

			lex.advance(); // Skip terminating quote.
		}


		void stringify_string(wpp::Lexer& lex, std::string& str, wpp::Env& env) {
			wpp::dbg("(parser) stringify_string");

			lex.advance(); // skip '`'.

			if (lex.peek() != TOKEN_IDENTIFIER)
				wpp::error(lex.position(), env, "expected an identifier to follow !.");


			str = lex.advance().str();
		}


		void raw_string(std::string&) {
			wpp::dbg("(parser) raw_string");
		}


		void para_string(std::string& str) {
			wpp::dbg("(parser) para_string");

			// Collapse consecutive runs of whitespace to a single whitespace.
			str.erase(std::unique(str.begin(), str.end(), [] (char lhs, char rhs) {
				return wpp::is_whitespace(&lhs) and wpp::is_whitespace(&rhs);
			}), str.end());

			// Replace all whitespace with a literal space.
			// So, newlines and tabs become spaces.
			for (char& c: str) {
				if (wpp::is_whitespace(&c))
					c = ' ';
			}

			// Strip leading and trailing whitespace.
			if (wpp::is_whitespace(&str.front()))
				str.erase(str.begin(), str.begin() + 1);

			if (wpp::is_whitespace(&str.back()))
				str.erase(str.end() - 1, str.end());
		}


		void code_string(std::string& str) {
			wpp::dbg("(parser) code_string");

			// Trim trailing whitespace.
			// Loop from back of string to beginning.
			for (auto it = str.rbegin(); it != str.rend(); ++it) {
				// If we find something that isn't whitespace, erase from the back
				// of the string to the current position of the iterator.
				if (not wpp::is_whitespace(&*it)) {
					str.erase(it.base(), str.end());
					break;
				}
			}

			// Trim leading whitespace.
			for (auto it = str.begin(); it != str.end();) {
				if (not wpp::is_whitespace(&*it))
					break;

				else if (*it == '\n') {
					str.erase(str.begin(), it + 1);
					it = str.begin();
					continue;  // Skip the increment of the iterator.
				}

				++it;
			}


			// Discover tab depth.
			int common_indent = std::numeric_limits<int>::max();

			for (auto it = str.begin(); *it; ++it) {
				if (*it == '\n') {
					int indent = 0;

					++it; // Skip newline.

					// Loop until we find something that isn't whitespace.
					while (wpp::is_whitespace(&*it))
						++it, ++indent;

					if (indent < common_indent)
						common_indent = indent;
				}
			}


			// Remove leading indentation on each line up to common_indent amount.

			// Strips whitespace until we hit either a non whitespace character
			// or reach the maximum amount of indentation to strip.
			const auto strip = [&] (const char* ptr) {
				const char* start = ptr;
				int count_whitespace = 0;

				while (wpp::is_whitespace(ptr) and count_whitespace != common_indent)
					++ptr, ++count_whitespace;

				str.erase(start - str.c_str(), ptr - start);

				// Set pointer back to beginning of removed range because
				// we are iterating while altering the string.
				ptr = start;

				return ptr;
			};

			const char* ptr = str.c_str();

			// Remove whitespace on first line between start of string and first non whitespace character.
			if (wpp::is_whitespace(ptr))
				ptr = strip(ptr);

			// Remove the rest of the leading whitespace.
			while (*ptr) {
				if (*ptr == '\n')
					ptr = strip(++ptr);

				++ptr;
			}
		}


		void hex_string(wpp::Lexer& lex, std::string& str, wpp::Env&) {
			wpp::dbg("(parser) hex_string");

			const auto& [ptr, len] = lex.advance().view;

			size_t counter = 0; // index into string, doesnt count `_`.

			for (size_t i = len; i > 0; i--) {
				const char c = ptr[i - 1];

				// Skip underscores.
				if (c == '_')
					continue;

				// Odd index, shift digit by 4 and or it with last character.
				if (counter & 1)
					str.back() |= wpp::hex_to_digit(c) << 4;

				// Even index, push back digit.
				else
					str.push_back(wpp::hex_to_digit(c));

				counter++;
			}

			std::reverse(str.begin(), str.end());
		}


		void bin_string(wpp::Lexer& lex, std::string& str, wpp::Env&) {
			wpp::dbg("(parser) bin_string");

			const auto& [ptr, len] = lex.advance().view;

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
		}


		void smart_string(wpp::Lexer& lex, std::string& str, wpp::Env& env) {
			wpp::dbg("(parser) smart_string");

			const auto tok = lex.advance(); // Consume the smart string opening token.

			const auto str_type = tok.view.at(0);  // 'r', 'p' or 'c'
			const auto delim = tok.view.at(1);  // User defined delimiter.

			const auto quote = lex.advance(wpp::lexer_modes::string); // ' or "

			while (true) {
				if (lex.peek(wpp::lexer_modes::string) == TOKEN_EOF)
					wpp::error(lex.position(), env, "reached EOF while parsing string.");

				// If we encounter ' or ", we check one character ahead to see
				// if it matches the user defined delimiter, it if does,
				// we erase the last quote character and break.
				if (lex.peek(wpp::lexer_modes::string) == quote) {
					// Consume this quote because it may actually be part of the
					// string and not the terminator.
					accumulate_string(lex.advance(wpp::lexer_modes::string), str);

					if (lex.peek(wpp::lexer_modes::chr).view == delim) {
						lex.advance(wpp::lexer_modes::chr); // Skip user delimiter.
						str.erase(str.end() - 1, str.end()); // Remove last quote.
						break;  // Exit the loop, string is fully consumed.
					}
				}

				// If not EOF or '/", consume.
				else {
					// We don't handle escape sequences inside a raw string.
					bool handle_escapes = str_type != 'r';
					accumulate_string(lex.advance(wpp::lexer_modes::string), str, handle_escapes);
				}
			}

			// From here, the different string types just make adjustments to the
			// contents of the parsed string.
			if (str_type == 'r')
				raw_string(str);

			else if (str_type == 'c')
				code_string(str);

			else if (str_type == 'p')
				para_string(str);
		}


		// Parse a string.
		// `"hey" 'hello' "a\nb\nc\n"`
		wpp::node_t string(wpp::Lexer& lex, wpp::AST& tree, wpp::Positions& pos, wpp::Env& env) {
			wpp::dbg("(parser) string");

			// Create our string node.
			const wpp::node_t node = tree.add<String>();
			pos.emplace_back(lex.position());
			auto& literal = tree.get<String>(node).value;

			// Reserve some space, this is kind of arbitrary.
			literal.reserve(1024);

			if (lex.peek() == TOKEN_HEX)
				hex_string(lex, literal, env);

			else if (lex.peek() == TOKEN_BIN)
				bin_string(lex, literal, env);

			else if (lex.peek() == TOKEN_SMART)
				smart_string(lex, literal, env);

			else if (lex.peek() == TOKEN_EXCLAIM)
				stringify_string(lex, literal, env);

			else if (lex.peek() == TOKEN_QUOTE or lex.peek() == TOKEN_DOUBLEQUOTE)
				normal_string(lex, literal, env);

			return node;
		}


		// Parse a function call.
		wpp::node_t fninvoke(wpp::Lexer& lex, wpp::AST& tree, wpp::Positions& pos, wpp::Env& env) {
			wpp::dbg("(parser) fninvoke");

			wpp::node_t node = tree.add<FnInvoke>();
			pos.emplace_back(lex.position());

			const auto fn_token = lex.advance();

			// Optional arguments.
			if (lex.peek() == TOKEN_LPAREN) {
				lex.advance();  // Skip `(`.

				// While there is an expression there is another parameter.
				while (peek_is_expr(lex.peek())) {
					// Parse expr.
					wpp::node_t expr = expression(lex, tree, pos, env);
					tree.get<FnInvoke>(node).arguments.emplace_back(expr);

					// If the next token is a comma, skip it.
					if (lex.peek() == TOKEN_COMMA)
						lex.advance(); // skip the comma

					// Otherwise it must be ')'?
					else if (lex.peek() != TOKEN_RPAREN)
						// If it's not, throw an exception.
						wpp::error(pos.at(node), env, "expecting comma to follow parameter name.");
				}

				// Make sure parameter list is terminated by `)`.
				if (lex.advance() != TOKEN_RPAREN)
					wpp::error(pos.at(node), env, "expecting ')' to follow argument list.");
			}

			// Check if function call is an intrinsic.

			// If it is an intrinsic, we replace the FnInvoke node type with
			// the Intrinsic node type and forward the arguments.
			if (peek_is_intrinsic(fn_token)) {
				const auto [_, args] = tree.get<FnInvoke>(node);
				tree.replace<Intrinsic>(node, fn_token.type, fn_token.view, args);
			}

			else
				tree.get<FnInvoke>(node).identifier = fn_token.view;

			return node;
		}


		wpp::node_t prefix(wpp::Lexer& lex, wpp::AST& tree, wpp::Positions& pos, wpp::Env& env) {
			wpp::dbg("(parser) prefix");

			// Create `Pre` node.
			const wpp::node_t node = tree.add<Pre>();
			pos.emplace_back(lex.position());


			// Skip `prefix` token, we already known it's there because
			// of it being seen by our caller, the statement parser.
			lex.advance();


			// Expect identifier.
			if (not peek_is_expr(lex.peek()))
				wpp::error(pos.at(node), env, "prefix does not have a name.");

			// Set name of `Pre`.
			const wpp::node_t expr = wpp::expression(lex, tree, pos, env);
			tree.get<Pre>(node).exprs = {expr};


			// Expect opening brace.
			if (lex.advance() != TOKEN_LBRACE)
				wpp::error(pos.at(node), env, "expecting '{' to follow name.");


			// Loop through body of prefix and collect statements.
			if (lex.peek() != TOKEN_RBRACE) {
				// Parse statement and then append it to statements vector in `Pre`.
				// The reason we seperate parsing and emplacing the node ID is
				// to prevent dereferencing an invalidated iterator.
				// If `tree` resizes while parsing the statement, by the time it returns
				// and is emplaced, the reference to the `Pre` node in `tree` might
				// be invalidated.
				do {
					const wpp::node_t stmt = statement(lex, tree, pos, env);
					tree.get<Pre>(node).statements.emplace_back(stmt);
				} while (peek_is_stmt(lex.peek()));
			}


			// Expect closing brace.
			if (lex.advance() != TOKEN_RBRACE)
				wpp::error(pos.at(node), env, "prefix is unterminated.");


			// Return index to `Pre` node we created at the top of this function.
			return node;
		}


		// Parse a block.
		wpp::node_t block(wpp::Lexer& lex, wpp::AST& tree, wpp::Positions& pos, wpp::Env& env) {
			wpp::dbg("(parser) block");

			const wpp::node_t node = tree.add<Block>();
			pos.emplace_back(lex.position());


			lex.advance(); // Skip '{'.

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

					const wpp::node_t stmt = statement(lex, tree, pos, env);
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
				wpp::error(pos.at(node), env, "expecting a trailing expression at the end of a block.");
			}

			if (lex.peek() == TOKEN_ARROW)
				wpp::error(pos.at(node), env, "map is missing test expression.");

			// Expect '}'.
			if (lex.peek() != TOKEN_RBRACE)
				wpp::error(pos.at(node), env, "block is unterminated.");

			lex.advance(); // Skip '}'.

			return node;
		}


		wpp::node_t map(wpp::Lexer& lex, wpp::AST& tree, wpp::Positions& pos, wpp::Env& env) {
			wpp::dbg("(parser) map");

			lex.advance(); // Skip `map`.

			const wpp::node_t node = tree.add<Map>();
			pos.emplace_back(lex.position());


			// Check for test expression.
			if (not peek_is_expr(lex.peek()))
				wpp::error(pos.at(node), env, "expected an expression to follow `map` keyword.");


			const auto expr = wpp::expression(lex, tree, pos, env); // Consume test expression.
			tree.get<Map>(node).expr = expr;


			if (lex.advance() != TOKEN_LBRACE)
				wpp::error(pos.at(node), env, "expected '{'.");


			// Collect all arms of the map.
			while (peek_is_expr(lex.peek())) {
				const auto arm = wpp::expression(lex, tree, pos, env);

				if (lex.advance() != TOKEN_ARROW)
					wpp::error(pos.at(node), env, "expected '->'.");

				if (not peek_is_expr(lex.peek()))
					wpp::error(pos.at(node), env, "expected expression.");

				const auto hand = wpp::expression(lex, tree, pos, env);

				tree.get<Map>(node).cases.emplace_back(std::pair{ arm, hand });
			}


			// Optional default case.
			if (lex.peek() == TOKEN_STAR) {
				lex.advance();

				if (lex.advance() != TOKEN_ARROW)
					wpp::error(pos.at(node), env, "expected '->'.");

				if (not peek_is_expr(lex.peek()))
					wpp::error(pos.at(node), env, "expected expression.");

				const auto default_case = wpp::expression(lex, tree, pos, env);
				tree.get<Map>(node).default_case = default_case;
			}

			else {
				tree.get<Map>(node).default_case = wpp::NODE_EMPTY;
			}


			if (lex.advance() != TOKEN_RBRACE)
				wpp::error(pos.at(node), env, "expected '}'.");

			return node;
		}


		// Parse an expression.
		wpp::node_t expression(wpp::Lexer& lex, wpp::AST& tree, wpp::Positions& pos, wpp::Env& env) {
			wpp::dbg("(parser) expression");

			// We use lhs to store the resulting expression
			// from the following cases and if the next token
			// is concatenation, we make a new Concat node using
			// lhs as the left hand side of the node and then continue on to
			// parse another expression for the right hand side.
			wpp::node_t lhs;

			const auto lookahead = lex.peek();

			if (peek_is_call(lookahead))
				lhs = wpp::fninvoke(lex, tree, pos, env);

			else if (peek_is_string(lookahead))
				lhs = wpp::string(lex, tree, pos, env);

			else if (lookahead == TOKEN_LBRACE)
				lhs = wpp::block(lex, tree, pos, env);

			else if (lookahead == TOKEN_MAP)
				lhs = wpp::map(lex, tree, pos, env);

			else if (lookahead == TOKEN_EQUAL)
				lhs = wpp::codeify(lex, tree, pos, env);

			else
				wpp::error(lex.position(), env, "expecting an expression.");

			if (lex.peek() == TOKEN_CAT) {
				const wpp::node_t node = tree.add<Concat>();
				pos.emplace_back(lex.position());

				lex.advance(); // Skip `..`.

				const wpp::node_t rhs = expression(lex, tree, pos, env);

				tree.get<Concat>(node).lhs = lhs;
				tree.get<Concat>(node).rhs = rhs;

				return node;
			}

			return lhs;
		}


		// Parse a statement.
		wpp::node_t statement(wpp::Lexer& lex, wpp::AST& tree, wpp::Positions& pos, wpp::Env& env) {
			wpp::dbg("(parser) statement");

			const auto lookahead = lex.peek();

			if (lookahead == TOKEN_LET)
				return wpp::let(lex, tree, pos, env);

			else if (lookahead == TOKEN_VAR)
				return wpp::var(lex, tree, pos, env);

			else if (lookahead == TOKEN_DROP)
				return wpp::drop(lex, tree, pos, env);

			else if (lookahead == TOKEN_PREFIX)
				return wpp::prefix(lex, tree, pos, env);

			else if (peek_is_expr(lookahead))
				return wpp::expression(lex, tree, pos, env);

			wpp::error(lex.position(), env, "expecting a statement.");
			return wpp::NODE_EMPTY;
		}
	}


	// Parse a document.
	// A document is just a series of zero or more expressions.
	wpp::node_t document(wpp::Lexer& lex, wpp::AST& tree, wpp::Positions& pos, wpp::Env& env) {
		wpp::dbg("(parser) document");

		const wpp::node_t node = tree.add<Document>();
		pos.emplace_back(lex.position());

		wpp::Error last;

		// Consume expressions until we encounter eof or an error.
		while (lex.peek() != TOKEN_EOF) {
			try {
				const wpp::node_t stmt = statement(lex, tree, pos, env);
				tree.get<Document>(node).stmts.emplace_back(stmt);
			}

			catch (const wpp::Error& e) {
				env.flags |= wpp::INTERNAL_ERROR_STATE;

				if (last == e)
					throw;

				last = e;
				e.show();
			}
		}

		return node;
	}
}
