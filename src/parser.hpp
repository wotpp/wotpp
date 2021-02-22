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
		wpp::token_type_t type;
		std::string identifier;
		std::vector<wpp::node_t> arguments;
		wpp::Position pos;

		Intrinsic(
			const wpp::token_type_t type_,
			const std::string& identifier_,
			const std::vector<wpp::node_t>& arguments_,
			const wpp::Position& pos_
		):
			type(type_),
			identifier(identifier_),
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

	struct Drop {
		wpp::node_t func;
		wpp::Position pos;

		Drop(const wpp::node_t& func_, const wpp::Position& pos_):
			func(func_), pos(pos_) {}

		Drop(const wpp::Position& pos_): pos(pos_) {}

		Drop() {}
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
		std::vector<wpp::node_t> exprs;
		std::vector<wpp::node_t> statements;
		wpp::Position pos;

		Pre(
			const std::vector<wpp::node_t>& exprs_,
			const std::vector<wpp::node_t>& statements_,
			const wpp::Position& pos_
		):
			exprs(exprs_),
			statements(statements_),
			pos(pos_) {}

		Pre(const wpp::Position& pos_): pos(pos_) {}

		Pre() {}
	};

	// Map strings to new strings.
	struct Map {
		wpp::node_t expr;
		std::vector<std::pair<wpp::node_t, wpp::node_t>> cases;
		wpp::node_t default_case;
		wpp::Position pos;

		Map(
			const wpp::node_t expr_,
			const std::vector<std::pair<wpp::node_t, wpp::node_t>>& cases_,
			const wpp::node_t default_case_,
			const wpp::Position& pos_
		):
			expr(expr_),
			cases(cases_),
			default_case(default_case_),
			pos(pos_) {}

		Map(const wpp::Position& pos_): pos(pos_) {}

		Map() {}
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
		Map,
		String,
		Concat,
		Block,
		Pre,
		Document,
		Drop
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
			tok == TOKEN_SLICE or
			tok == TOKEN_FIND or
			tok == TOKEN_LENGTH or
			tok == TOKEN_ESCAPE or
			tok == TOKEN_LOG
		;
	}

	inline bool peek_is_keyword(const wpp::Token& tok) {
		return
			tok == TOKEN_LET or
			tok == TOKEN_PREFIX or
			tok == TOKEN_DROP
		;
	}

	// Check if the token is a string.
	inline bool peek_is_string(const wpp::Token& tok) {
		return
			tok == TOKEN_DOUBLEQUOTE or
			tok == TOKEN_QUOTE or

			tok == TOKEN_BACKTICK or

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
			tok == TOKEN_MAP or
			tok == TOKEN_LBRACE or
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








	// Consume tokens comprising a string. Handles escape chars.
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





	// Forward declarations.
	inline wpp::node_t string(wpp::Lexer&, wpp::AST&);

	inline void normal_string(wpp::Lexer&, std::string&);
	inline void backtick_string(wpp::Lexer&, std::string&);
	inline void smart_string(wpp::Lexer&, std::string&);
	inline void hex_string(wpp::Lexer&, std::string&);
	inline void bin_string(wpp::Lexer&, std::string&);

	inline void raw_string(std::string&);
	inline void para_string(std::string&);
	inline void code_string(std::string&);

	inline wpp::node_t function(wpp::Lexer&, wpp::AST&);
	inline wpp::node_t drop(wpp::Lexer&, wpp::AST&);
	inline wpp::node_t call(wpp::Lexer&, wpp::AST&);
	inline wpp::node_t prefix(wpp::Lexer&, wpp::AST&);
	inline wpp::node_t block(wpp::Lexer&, wpp::AST&);
	inline wpp::node_t expression(wpp::Lexer&, wpp::AST&);
	inline wpp::node_t map(wpp::Lexer&, wpp::AST&);
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


			// While there is an identifier there is another parameter.
			while (lex.peek() == TOKEN_IDENTIFIER) {
				// Advance the lexer and get the identifier.
				auto id = lex.advance().str();

				// Add the argument
				tree.get<Fn>(node).parameters.emplace_back(id);

				// If the next token is a comma, skip it.
				if (lex.peek() == TOKEN_COMMA)
					lex.advance(); // skip the comma

				// Otherwise it must be ')'?
				else if (lex.peek() != TOKEN_RPAREN)
					// If it's not, throw an exception.
					throw wpp::Exception{lex.position(), "expecting comma to follow parameter name."};
			}

			// Check if there's an keyword conflict.
			// We check if the next token is a reserved name and throw an error
			// if it is. The reason we don't check this in the while loop body is
			// because the loop condition checks for an identifier and so breaks
			// out if the next token is an intrinsic.
			if (peek_is_reserved_name(lex.peek()))
				throw wpp::Exception{lex.position(), "parameter name '", lex.advance().str(), "' conflicts with keyword."};

			// Make sure parameter list is terminated by `)`.
			if (lex.advance() != TOKEN_RPAREN)
				throw wpp::Exception{lex.position(), "expecting ')' to follow argument list."};
		}

		// Parse the function body.
		const wpp::node_t body = expression(lex, tree);
		tree.get<Fn>(node).body = body;

		return node;
	}


	inline wpp::node_t drop(wpp::Lexer& lex, wpp::AST& tree) {
		lex.advance(); // Skip `drop`.

		const wpp::node_t node = tree.add<Drop>(lex.position());

		const wpp::node_t call_expr = wpp::call(lex, tree);
		tree.get<Drop>(node).func = call_expr;

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


	inline void backtick_string(wpp::Lexer& lex, std::string& str) {
		lex.advance(); // skip '`'.

		if (lex.peek() != TOKEN_IDENTIFIER)
			throw wpp::Exception{lex.position(), "expected an identifier to follow `."};

		str = lex.advance().str();
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

		else if (lex.peek() == TOKEN_BACKTICK)
			backtick_string(lex, literal);

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
			lex.advance();  // Skip `(`.

			// While there is an expression there is another parameter.
			while (peek_is_expr(lex.peek())) {
				// Parse expr.
				wpp::node_t expr = expression(lex, tree);
				tree.get<FnInvoke>(node).arguments.emplace_back(expr);

				// If the next token is a comma, skip it.
				if (lex.peek() == TOKEN_COMMA)
					lex.advance(); // skip the comma

				// Otherwise it must be ')'?
				else if (lex.peek() != TOKEN_RPAREN)
					// If it's not, throw an exception.
					throw wpp::Exception{lex.position(), "expecting comma to follow parameter name."};
			}

			// Make sure parameter list is terminated by `)`.
			if (lex.advance() != TOKEN_RPAREN)
				throw wpp::Exception{lex.position(), "expecting ')' to follow argument list."};
		}

		// Check if function call is an intrinsic.

		// If it is an intrinsic, we replace the FnInvoke node type with
		// the Intrinsic node type and forward the arguments.
		if (peek_is_intrinsic(fn_token)) {
			const auto [_, args, pos] = tree.get<FnInvoke>(node);
			tree.replace<Intrinsic>(node, fn_token.type, fn_token.str(), args, pos);
		}

		else
			tree.get<FnInvoke>(node).identifier = fn_token.str();

		return node;
	}


	inline wpp::node_t prefix(wpp::Lexer& lex, wpp::AST& tree) {
		// Create `Pre` node.
		const wpp::node_t node = tree.add<Pre>(lex.position());


		// Skip `prefix` token, we already known it's there because
		// of it being seen by our caller, the statement parser.
		lex.advance();


		// Expect identifier.
		if (not peek_is_expr(lex.peek()))
			throw wpp::Exception{lex.position(), "prefix does not have a name."};

		// Set name of `Pre`.
		const wpp::node_t expr = wpp::expression(lex, tree);
		tree.get<Pre>(node).exprs = {expr};


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

				const wpp::node_t stmt = statement(lex, tree);
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
			throw wpp::Exception{lex.position(), "expecting a trailing expression at the end of a block"};
		}

		if (lex.peek() == TOKEN_ARROW)
			throw wpp::Exception{tree.get<Block>(node).pos, "map is missing test expression."};

		// Expect '}'.
		if (lex.peek() != TOKEN_RBRACE)
			throw wpp::Exception{lex.position(), "block is unterminated."};

		lex.advance(); // Skip '}'.

		return node;
	}


	inline wpp::node_t map(wpp::Lexer& lex, wpp::AST& tree) {
		lex.advance(); // Skip `map`.

		const wpp::node_t node = tree.add<Map>(lex.position());

		// Check for test expression.
		if (not peek_is_expr(lex.peek()))
			throw wpp::Exception{lex.position(), "expected an expression to follow `map` keyword."};


		const auto expr = wpp::expression(lex, tree); // Consume test expression.
		tree.get<Map>(node).expr = expr;


		if (lex.advance() != TOKEN_LBRACE)
			throw wpp::Exception{lex.position(), "expected '{'."};


		// Collect all arms of the map.
		while (peek_is_expr(lex.peek())) {
			const auto arm = wpp::expression(lex, tree);

			if (lex.advance() != TOKEN_ARROW)
				throw wpp::Exception{lex.position(), "expected '->'."};

			if (not peek_is_expr(lex.peek()))
				throw wpp::Exception{lex.position(), "expected expression."};

			const auto hand = wpp::expression(lex, tree);

			tree.get<Map>(node).cases.emplace_back(std::pair{ arm, hand });
		}


		// Optional default case.
		if (lex.peek() == TOKEN_STAR) {
			lex.advance();

			if (lex.advance() != TOKEN_ARROW)
				throw wpp::Exception{lex.position(), "expected '->'."};

			if (not peek_is_expr(lex.peek()))
				throw wpp::Exception{lex.position(), "expected expression."};

			const auto default_case = wpp::expression(lex, tree);
			tree.get<Map>(node).default_case = default_case;
		}

		else {
			tree.get<Map>(node).default_case = wpp::NODE_EMPTY;
		}


		if (lex.advance() != TOKEN_RBRACE)
			throw wpp::Exception{lex.position(), "expected '}'."};

		return node;
	}


	// Parse an expression.
	inline wpp::node_t expression(wpp::Lexer& lex, wpp::AST& tree) {
		// We use lhs to store the resulting expression
		// from the following cases and if the next token
		// is concatenation, we make a new Concat node using
		// lhs as the left hand side of the node and then continue on to
		// parse another expression for the right hand side.
		wpp::node_t lhs;

		const auto lookahead = lex.peek();

		if (peek_is_call(lookahead))
			lhs = wpp::call(lex, tree);

		else if (peek_is_string(lookahead))
			lhs = wpp::string(lex, tree);

		else if (lookahead == TOKEN_LBRACE)
			lhs = wpp::block(lex, tree);

		else if (lookahead == TOKEN_MAP)
			lhs = wpp::map(lex, tree);

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

		if (lookahead == TOKEN_LET)
			return wpp::function(lex, tree);

		else if (lookahead == TOKEN_DROP)
			return wpp::drop(lex, tree);

		else if (lookahead == TOKEN_PREFIX)
			return wpp::prefix(lex, tree);

		else if (peek_is_expr(lookahead))
			return wpp::expression(lex, tree);

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
