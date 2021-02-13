#pragma once

#ifndef WOTPP_PARSER
#define WOTPP_PARSER

#include <vector>
#include <algorithm>
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
		std::vector<wpp::node_t> arguments;
		wpp::Position pos;

		Intrinsic(
			const uint8_t type_,
			const std::vector<wpp::node_t>& arguments_,
			const wpp::Position& pos_
		):
			type(type_),
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
			tok == TOKEN_INCLUDE or
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

			tok == TOKEN_RAW or
			tok == TOKEN_PARA
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




	// Forward declarations.
	inline wpp::node_t string(wpp::Lexer&, wpp::AST&);
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


	// Parse a string.
	// `"hey" 'hello' "a\nb\nc\n"`
	inline wpp::node_t string(wpp::Lexer& lex, wpp::AST& tree) {
		// Create our string node.
		const wpp::node_t node = tree.add<String>(lex.position());
		auto& [literal, pos] = tree.get<String>(node);

		const auto hex_digit = [] (char c) -> int {
			if (c >= '0' && c <= '9')
				return c - '0';

			if (c >= 'a' && c <= 'f')
				return c - 'a' + 10;

			if (c >= 'A' && c <= 'F')
				return c - 'A' + 10;

			return '\0';
		};

		// 0010101010101010___10101010101__10100
		if (lex.peek() == TOKEN_HEX) {
			const auto& [ptr, len] = lex.advance().view;

			size_t j = 0;
			for (size_t i = len; i > 0; i--) {
				const char c = ptr[i - 1];

				if (c == '_')
					continue;

				if (j & 1)
					literal.back() |= hex_digit(c) << 4;

				else
					literal.push_back(hex_digit(c));

				j++;
			}

			std::reverse(literal.begin(), literal.end());
		}

		else if (lex.peek() == TOKEN_BIN) {
			const auto& [ptr, len] = lex.advance().view;

			size_t j = 0;
			for (size_t i = len; i > 0; i--) {
				const char c = ptr[i - 1];

				if (c == '_')
					continue;

				if (j & 7)
					literal.back() |= (c - '0') << (j & 7);

				else
					literal.push_back(c - '0');

				j++;
			}

			std::reverse(literal.begin(), literal.end());
		}

		else {
			const auto delim = lex.advance(wpp::modes::string); // Store delimeter.

			// Reserve some space, this is kind of arbitrary.
			literal.reserve(1024);

			// Consume tokens until we reach `delim` or EOF.
			while (lex.peek(wpp::modes::string) != delim) {
				if (lex.peek(wpp::modes::string) == TOKEN_EOF)
					throw wpp::Exception{lex.position(), "reached EOF while parsing string."};

				const auto part = lex.advance(wpp::modes::string);

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

				else if (part == TOKEN_ESCAPE_HEX) {
					uint8_t first_nibble = *part.view.ptr;
					uint8_t second_nibble = *(part.view.ptr + 1);

					literal += static_cast<uint8_t>(hex_digit(first_nibble) << 4 | hex_digit(second_nibble));
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

			lex.advance(); // consume end quote
		}

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

		if (peek_is_intrinsic(fn_token)) {
			tree.replace<Intrinsic>(node, fn_token.type, args, pos);
		}

		else {
			tree.get<FnInvoke>(node).identifier = fn_token.str();
		}

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
			// if (peek_is_stmt(lex.peek())) {
				const wpp::node_t stmt = statement(lex, tree);
				tree.get<Document>(node).stmts.emplace_back(stmt);
			// }

			// else {
			// 	throw wpp::Exception{lex.position(), "expecting a statement."};
			// }
		}

		return node;
	}
}


#endif
