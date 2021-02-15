#pragma once

#ifndef WOTPP_PARSER
#define WOTPP_PARSER

#include <vector>
#include <algorithm>
#include <limits>
#include <string>

#include <utils/util.hpp>
#include <structures/ast.hpp>
#include <structures/token.hpp>
#include <structures/position.hpp>
#include <exception.hpp>
#include <lexer.hpp>

// The meat of wot++, the parser.
// This is a plain old LL(1) predictive recursive descent parser.

// AST nodes.
namespace wpp {
	// A function call.
	struct FnInvoke {
		std::string identifier;
		std::vector<wpp::node_t> arguments;
		wpp::Position pos;

		FnInvoke(
			const std::string& identifier_,
			const std::vector<wpp::node_t>& arguments_,
			const wpp::Position& pos_
		):
			identifier(identifier_),
			arguments(arguments_),
			pos(pos_) {}

		FnInvoke(const wpp::Position& pos_): pos(pos_) {}

		FnInvoke() {}
	};

	struct Intrinsic {
		uint8_t type;
		std::string identifer;
		std::vector<wpp::node_t> arguments;
		wpp::Position pos;

		Intrinsic(
			const uint8_t type_,
			const std::string& identifier_,
			const std::vector<wpp::node_t>& arguments_,
			const wpp::Position& pos_
		):
			type(type_),
			identifer(identifier_),
			arguments(arguments_),
			pos(pos_) {}

		Intrinsic(const wpp::Position& pos_): pos(pos_) {}

		Intrinsic() {}
	};

	// Function definition.
	struct Fn {
		std::string identifier;
		std::vector<std::string> parameters;
		wpp::node_t body;
		wpp::Position pos;

		Fn(
			const std::string& identifier_,
			const std::vector<std::string>& parameters_,
			const wpp::node_t body_,
			const wpp::Position& pos_
		):
			identifier(identifier_),
			parameters(parameters_),
			body(body_),
			pos(pos_) {}

		Fn(const wpp::Position& pos_): pos(pos_) {}

		Fn() {}
	};

	// String literal.
	struct String {
		std::string value;
		wpp::Position pos;

		String(const std::string& value_, const wpp::Position& pos_):
			value(value_), pos(pos_) {}

		String(const wpp::Position& pos_): pos(pos_) {}

		String() {}
	};

	// Concatenation operator.
	struct Concat {
		wpp::node_t lhs, rhs;
		wpp::Position pos;

		Concat(wpp::node_t lhs_, wpp::node_t rhs_, const wpp::Position& pos_):
			lhs(lhs_), rhs(rhs_), pos(pos_) {}

		Concat(const wpp::Position& pos_): pos(pos_) {}

		Concat() {}
	};

	// Block of zero or more statements and trailing expression.
	struct Block {
		std::vector<wpp::node_t> statements;
		wpp::node_t expr;
		wpp::Position pos;

		Block(
			const std::vector<wpp::node_t>& statements_,
			const wpp::node_t expr_,
			const wpp::Position& pos_
		):
			statements(statements_),
			expr(expr_),
			pos(pos_) {}

		Block(const wpp::Position& pos_): pos(pos_) {}

		Block() {}
	};

	// Namespace that embodies zero or more statements.
	struct Pre {
		std::string identifier;
		std::vector<wpp::node_t> statements;
		wpp::Position pos;

		Pre(
			const std::string& identifier_,
			const std::vector<wpp::node_t>& statements_,
			const wpp::Position& pos_
		):
			identifier(identifier_),
			statements(statements_),
			pos(pos_) {}

		Pre(const wpp::Position& pos_): pos(pos_) {}

		Pre() {}
	};

	// The root node of a wot++ program.
	struct Document {
		std::vector<wpp::node_t> stmts;
		wpp::Position pos;

		Document(
			const std::vector<wpp::node_t>& stmts_,
			const wpp::Position& pos_
		):
			stmts(stmts_),
			pos(pos_) {}

		Document(const wpp::Position& pos_): pos(pos_) {}

		Document() {}
	};

	// An alias for our AST.
	using AST = wpp::HeterogenousVector<
		FnInvoke,
		Intrinsic,
		Fn,
		String,
		Concat,
		Block,
		Pre,
		Document
	>;
}


namespace wpp {
	inline bool peek_is_intrinsic(const wpp::Token& tok) {
		return
			tok == TOKEN_RUN or
			tok == TOKEN_EVAL or
			tok == TOKEN_FILE or
			tok == TOKEN_ASSERT or
			tok == TOKEN_PIPE or
			tok == TOKEN_ERROR or
			tok == TOKEN_SOURCE or
			tok == TOKEN_ESCAPE
		;
	}

	inline bool peek_is_keyword(const wpp::Token& tok) {
		return
			tok == TOKEN_LET or
			tok == TOKEN_PREFIX
		;
	}

	// Check if the token is a string.
	inline bool peek_is_string(const wpp::Token& tok) {
		return
			tok == TOKEN_DOUBLEQUOTE or
			tok == TOKEN_QUOTE or

			tok == TOKEN_HEX or
			tok == TOKEN_BIN or

			tok == TOKEN_SMART
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
			tok == TOKEN_IDENTIFIER or
			peek_is_intrinsic(tok)
		;
	}

	// Check if the token is an expression.
	inline bool peek_is_expr(const wpp::Token& tok) {
		return
			peek_is_string(tok) or
			tok == TOKEN_LBRACE or
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








	void accumulate_string(const wpp::Token& part, std::string& literal) {
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
			uint8_t first_nibble = *part.view.ptr;
			uint8_t second_nibble = *(part.view.ptr + 1);

			literal += static_cast<uint8_t>(wpp::hex_to_digit(first_nibble) << 4 | wpp::hex_to_digit(second_nibble));
		}

		else if (part == TOKEN_ESCAPE_BIN) {
			const auto& [ptr, len] = part.view;
			uint8_t value = 0;

			for (size_t i = 0; i < len; ++i) {
				value <<= 1;
				value |= ptr[i] - '0';
			}

			literal += value;
		}

		else
			literal.append(part.str());
	}





	// Forward declarations.
	inline wpp::node_t string(wpp::Lexer&, wpp::AST&);

	inline void normal_string(wpp::Lexer&, std::string&);
	inline void smart_string(wpp::Lexer&, std::string&);
	inline void hex_string(wpp::Lexer&, std::string&);
	inline void bin_string(wpp::Lexer&, std::string&);

	inline void raw_string(std::string&);
	inline void para_string(std::string&);
	inline void code_string(std::string&);

	inline wpp::node_t function(wpp::Lexer&, wpp::AST&);
	inline wpp::node_t call(wpp::Lexer&, wpp::AST&);
	inline wpp::node_t nspace(wpp::Lexer&, wpp::AST&);
	inline wpp::node_t block(wpp::Lexer&, wpp::AST&);
	inline wpp::node_t expression(wpp::Lexer&, wpp::AST&);
	inline wpp::node_t statement(wpp::Lexer&, wpp::AST&);
	inline wpp::node_t document(wpp::Lexer&, wpp::AST&);




	// Parses a function.
	inline wpp::node_t function(wpp::Lexer& lex, wpp::AST& tree) {
		// Create `Fn` node ahead of time so we can insert member data
		// directly instead of copying/moving it into a new node at the end.
		const wpp::node_t node = tree.add<Fn>(lex.position());

		// Skip `let` keyword. The statement parser already checked
		// for it before calling us.
		lex.advance();


		// Make sure the next token is an identifier, if it is, set the name
		// of our `Fn` node to match.
		if (lex.peek() != TOKEN_IDENTIFIER)
			throw wpp::Exception{lex.position(), "function declaration does not have a name."};

		tree.get<Fn>(node).identifier = lex.advance().str();


		// Collect parameters.
		// Advance until we run out of identifiers.
		if (lex.peek() == TOKEN_LPAREN) {
			lex.advance();  // Skip `(`.


			// Check for the first parameter.
			if (lex.peek() == TOKEN_IDENTIFIER)
				tree.get<Fn>(node).parameters.emplace_back(lex.advance().str());

			else if (peek_is_reserved_name(lex.peek()))
				throw wpp::Exception{lex.position(), "parameter name '", lex.advance().str(), "' conflicts with keyword."};


			// While there is a comma, loop until we run out of parameters.
			while (lex.peek() == TOKEN_COMMA) {
				lex.advance(); // Skip `,`.

				// Allow trailing comma.
				if (lex.peek() == TOKEN_RPAREN)
					break;


				// Check if there's an keyword conflict.
				if (peek_is_reserved_name(lex.peek()))
					throw wpp::Exception{lex.position(), "parameter name '", lex.advance().str(), "' conflicts with keyword."};


				// Check if there is a parameter.
				if (lex.peek() != TOKEN_IDENTIFIER)
					throw wpp::Exception{lex.position(), "expecting parameter name to follow comma."};


				// Emplace the parameter name into our `Fn` node's list of params.
				tree.get<Fn>(node).parameters.emplace_back(lex.advance().str());
			}


			// Make sure parameter list is terminated by `)`.
			if (lex.advance() != TOKEN_RPAREN)
				throw wpp::Exception{lex.position(), "parameter list is unterminated."};
		}

		// Parse the function body.
		const wpp::node_t body = expression(lex, tree);
		tree.get<Fn>(node).body = body;

		return node;
	}


	inline void normal_string(wpp::Lexer& lex, std::string& str) {
		const auto delim = lex.advance(wpp::modes::string); // Store delimeter.

		// Consume tokens until we reach `delim` or EOF.
		while (lex.peek(wpp::modes::string) != delim) {
			if (lex.peek(wpp::modes::string) == TOKEN_EOF)
				throw wpp::Exception{lex.position(), "reached EOF while parsing string."};

			// Parse escape characters and append "parts" of the string to `str`.
			accumulate_string(lex.advance(wpp::modes::string), str);
		}

		lex.advance(); // Skip terminating quote.
	}


	inline void raw_string(std::string&) {

	}


	inline void para_string(std::string& str) {
		// Collapse consecutive runs of whitespace to a single whitespace.
		str.erase(std::unique(str.begin(), str.end(), [] (char lhs, char rhs) {
			return wpp::is_whitespace(lhs) and wpp::is_whitespace(rhs);
		}), str.end());

		// Replace all whitespace with a literal space.
		// So, newlines and tabs become spaces.
		for (char& c: str) {
			if (wpp::is_whitespace(c))
				c = ' ';
		}

		// Strip leading and trailing whitespace.
		if (wpp::is_whitespace(str.front()))
			str.erase(str.begin(), str.begin() + 1);

		if (wpp::is_whitespace(str.back()))
			str.erase(str.end() - 1, str.end());
	}


	inline void code_string(std::string& str) {
		// Trim trailing whitespace.
		// Loop from back of string to beginning.
		for (auto it = str.rbegin(); it != str.rend(); ++it) {
			// If we find something that isn't whitespace, erase from the back
			// of the string to the current position of the iterator.
			if (not wpp::is_whitespace(*it)) {
				str.erase(it.base(), str.end());
				break;
			}
		}

		// Trim leading whitespace.
		for (auto it = str.begin(); it != str.end();) {
			if (not wpp::is_whitespace(*it))
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
				while (wpp::is_whitespace(*it))
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

			while (wpp::is_whitespace(*ptr) and count_whitespace != common_indent)
				++ptr, ++count_whitespace;

			str.erase(start - str.c_str(), ptr - start);

			// Set pointer back to beginning of removed range because
			// we are iterating while altering the string.
			ptr = start;

			return ptr;
		};

		const char* ptr = str.c_str();

		// Remove whitespace on first line between start of string and first non whitespace character.
		if (wpp::is_whitespace(*ptr))
			ptr = strip(ptr);

		// Remove the rest of the leading whitespace.
		while (*ptr) {
			if (*ptr == '\n')
				ptr = strip(++ptr);

			++ptr;
		}
	}


	inline void hex_string(wpp::Lexer& lex, std::string& str) {
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


	inline void bin_string(wpp::Lexer& lex, std::string& str) {
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


	inline void smart_string(wpp::Lexer& lex, std::string& str) {
		const auto tok = lex.advance(); // Consume the smart string opening token.

		const auto str_type = tok.view.at(0);  // 'r', 'p' or 'c'
		const auto delim = tok.view.at(1);  // User defined delimiter.

		const auto quote = lex.advance(wpp::modes::string); // ' or "

		while (true) {
			if (lex.peek(wpp::modes::string) == TOKEN_EOF)
				throw wpp::Exception{lex.position(), "reached EOF while parsing string."};

			// If we encounter ' or ", we check one character ahead to see
			// if it matches the user defined delimiter, it if does,
			// we erase the last quote character and break.
			else if (lex.peek(wpp::modes::string) == quote) {
				// Consume this quote because it may actually be part of the
				// string and not the terminator.
				accumulate_string(lex.advance(wpp::modes::string), str);

				if (lex.peek(wpp::modes::character).view == delim) {
					lex.advance(wpp::modes::character); // Skip user delimiter.
					str.erase(str.end() - 1, str.end()); // Remove last quote.
					break;  // Exit the loop, string is fully consumed.
				}
			}

			// If not EOF or '/", consume.
			else {
				accumulate_string(lex.advance(wpp::modes::string), str);
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
	inline wpp::node_t string(wpp::Lexer& lex, wpp::AST& tree) {
		// Create our string node.
		const wpp::node_t node = tree.add<String>(lex.position());
		auto& [literal, pos] = tree.get<String>(node);

		// Reserve some space, this is kind of arbitrary.
		literal.reserve(1024);

		if (lex.peek() == TOKEN_HEX)
			hex_string(lex, literal);

		else if (lex.peek() == TOKEN_BIN)
			bin_string(lex, literal);

		else if (lex.peek() == TOKEN_SMART)
			smart_string(lex, literal);

		else if (lex.peek() == TOKEN_QUOTE or lex.peek() == TOKEN_DOUBLEQUOTE)
			normal_string(lex, literal);

		return node;
	}


	// Parse a function call.
	inline wpp::node_t call(wpp::Lexer& lex, wpp::AST& tree) {
		wpp::node_t node = tree.add<FnInvoke>(lex.position());
		const auto fn_token = lex.advance();

		// Optional arguments.
		if (lex.peek() == TOKEN_LPAREN) {
			lex.advance(); // skip lparen.

			if (lex.peek() != TOKEN_RPAREN) {
				// throw wpp::Exception{lex.position(), "empty arguments, drop the parens."};

				// Collect arguments.
				wpp::node_t expr;

				expr = expression(lex, tree);
				tree.get<FnInvoke>(node).arguments.emplace_back(expr);

				while (lex.peek() == TOKEN_COMMA) {
					lex.advance(); // skip comma.

					// Allow trailing comma.
					if (lex.peek() == TOKEN_RPAREN)
						break;

					expr = expression(lex, tree);
					tree.get<FnInvoke>(node).arguments.emplace_back(expr);
				}

				if (lex.advance() != TOKEN_RPAREN)
					throw wpp::Exception{lex.position(), "expecting closing parenthesis after argument list."};
			}

			else {
				lex.advance(); // skip `)`
			}
		}

		const auto [_, args, pos] = tree.get<FnInvoke>(node);

		if (peek_is_intrinsic(fn_token))
			tree.replace<Intrinsic>(node, fn_token.type, fn_token.str(), args, pos);

		else
			tree.get<FnInvoke>(node).identifier = fn_token.str();

		return node;
	}


	inline wpp::node_t nspace(wpp::Lexer& lex, wpp::AST& tree) {
		// Create `Pre` node.
		const wpp::node_t node = tree.add<Pre>(lex.position());


		// Skip `prefix` token, we already known it's there because
		// of it being seen by our caller, the statement parser.
		lex.advance();


		// Expect identifier.
		if (lex.peek() != TOKEN_IDENTIFIER)
			throw wpp::Exception{lex.position(), "prefix does not have a name."};

		// Set name of `Pre`.
		tree.get<Pre>(node).identifier = lex.advance().str();


		// Expect opening brace.
		if (lex.advance() != TOKEN_LBRACE)
			throw wpp::Exception{lex.position(), "expecting '{' to follow name."};


		// Loop through body of prefix and collect statements.
		if (lex.peek() != TOKEN_RBRACE) {
			// Parse statement and then append it to statements vector in `Pre`.
			// The reason we seperate parsing and emplacing the node ID is
			// to prevent dereferencing an invalidated iterator.
			// If `tree` resizes while parsing the statement, by the time it returns
			// and is emplaced, the reference to the `Pre` node in `tree` might
			// be invalidated.
			do {
				const wpp::node_t stmt = statement(lex, tree);
				tree.get<Pre>(node).statements.emplace_back(stmt);
			} while (peek_is_stmt(lex.peek()));
		}


		// Expect closing brace.
		if (lex.advance() != TOKEN_RBRACE)
			throw wpp::Exception{lex.position(), "prefix is unterminated."};


		// Return index to `Pre` node we created at the top of this function.
		return node;
	}


	// Parse a block.
	inline wpp::node_t block(wpp::Lexer& lex, wpp::AST& tree) {
		const wpp::node_t node = tree.add<Block>(lex.position());

		lex.advance(); // skip lbrace

		// check if theres a statement, otherwise its just a single expression
		bool last_is_expr = false;
		if (peek_is_stmt(lex.peek())) {
			do {
				if (peek_is_expr(lex.peek()))
					last_is_expr = true;

				else
					last_is_expr = false;

				const wpp::node_t stmt = statement(lex, tree);
				tree.get<Block>(node).statements.emplace_back(stmt);
			} while (peek_is_stmt(lex.peek()));
		}

		if (not peek_is_expr(lex.peek()) and last_is_expr) {
			tree.get<Block>(node).expr = tree.get<Block>(node).statements.back();
			tree.get<Block>(node).statements.pop_back();
		}

		else if (peek_is_expr(lex.peek()) and not last_is_expr) {
			const wpp::node_t expr = expression(lex, tree);
			tree.get<Block>(node).expr = expr;
		}

		else {
			throw wpp::Exception{lex.position(), "expecting a trailing expression at the end of a block"};
		}

		if (lex.advance() != TOKEN_RBRACE)
			throw wpp::Exception{lex.position(), "block is unterminated."};

		return node;
	}


	// Parse an expression.
	inline wpp::node_t expression(wpp::Lexer& lex, wpp::AST& tree) {
		wpp::node_t lhs;

		const auto lookahead = lex.peek();

		if (peek_is_call(lookahead))
			lhs = wpp::call(lex, tree);

		else if (peek_is_string(lookahead))
			lhs = wpp::string(lex, tree);

		else if (lookahead == TOKEN_LBRACE)
			lhs = wpp::block(lex, tree);

		else
			throw wpp::Exception{lex.position(), "expecting an expression."};

		if (lex.peek() == TOKEN_CAT) {
			const wpp::node_t node = tree.add<Concat>(lex.position());

			lex.advance(); // Skip `..`.

			const wpp::node_t rhs = expression(lex, tree);

			tree.get<Concat>(node).lhs = lhs;
			tree.get<Concat>(node).rhs = rhs;

			return node;
		}

		return lhs;
	}


	// Parse a statement.
	inline wpp::node_t statement(wpp::Lexer& lex, wpp::AST& tree) {
		const auto lookahead = lex.peek();

		// function declaration.
		if (lookahead == TOKEN_LET)
			return wpp::function(lex, tree);

		// prefix
		else if (lookahead == TOKEN_PREFIX)
			return wpp::nspace(lex, tree);

		else if (peek_is_expr(lookahead)) {
			return wpp::expression(lex, tree);
		};

		throw wpp::Exception{lex.position(), "expecting a statement."};
	}


	// Parse a document.
	// A document is just a series of zero or more expressions.
	inline wpp::node_t document(wpp::Lexer& lex, wpp::AST& tree) {
		const wpp::node_t node = tree.add<Document>(lex.position());

		// Consume expressions until we encounter eof or an error.
		while (lex.peek() != TOKEN_EOF) {
			const wpp::node_t stmt = statement(lex, tree);
			tree.get<Document>(node).stmts.emplace_back(stmt);
		}

		return node;
	}
}

#endif
