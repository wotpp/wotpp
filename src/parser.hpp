#pragma once

#ifndef WOTPP_PARSER
#define WOTPP_PARSER

#include <vector>
#include <string>

#include <utils/util.hpp>
#include <structures/ast.hpp>
#include <structures/token.hpp>
#include <structures/position.hpp>
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

		FnInvoke(const std::string& identifier_, const std::vector<wpp::node_t>& arguments_, wpp::Position pos_):
			identifier(identifier_), arguments(arguments_), pos(pos_) {}

		FnInvoke(wpp::Position pos_): pos(pos_) {}
	};

	// Function definition.
	struct Fn {
		std::string identifier;
		std::vector<std::string> parameters;
		wpp::node_t body;

		Fn(const std::string& identifier_, const std::vector<std::string>& parameters_):
			identifier(identifier_), parameters(parameters_) {}

		Fn() {}
	};

	// String literal.
	struct String {
		std::string value;

		String(const std::string& value_):
			value(value_) {}

		String() {}
	};

	// Concatenation operator.
	struct Concat {
		wpp::node_t lhs, rhs;

		Concat(wpp::node_t lhs_, wpp::node_t rhs_):
			lhs(lhs_), rhs(rhs_) {}

		Concat() {}
	};

	// Block of zero or more statements and trailing expression.
	struct Block {
		std::vector<wpp::node_t> statements;
		wpp::node_t expr;

		Block(const std::vector<wpp::node_t>& statements_, wpp::node_t expr_):
			statements(statements_), expr(expr_) {}

		Block() {}
	};

	// Namespace that embodies zero or more statements.
	struct Ns {
		std::string identifier;
		std::vector<wpp::node_t> statements;

		Ns(const std::string& identifier_, const std::vector<wpp::node_t>& statements_):
			identifier(identifier_), statements(statements_) {}

		Ns() {}
	};

	// The root node of a wot++ program.
	struct Document {
		std::vector<wpp::node_t> exprs_or_stmts;

		Document(const std::vector<wpp::node_t>& exprs_or_stmts_):
			exprs_or_stmts(exprs_or_stmts_) {}

		Document() {}
	};

	// An alias for our AST.
	using AST = wpp::HomogenousVector<FnInvoke, Fn, String, Concat, Block, Ns, Document>;
}


namespace wpp {
	// Check if the token is a statement.
	inline bool peek_is_stmt(const wpp::Token& tok) {
		return
			tok == TOKEN_LET or
			tok == TOKEN_NAMESPACE
		;
	}

	// Check if the token is an expression.
	inline bool peek_is_expr(const wpp::Token& tok) {
		return
			tok == TOKEN_DOUBLEQUOTE or
			tok == TOKEN_QUOTE or
			tok == TOKEN_LBRACE or
			tok == TOKEN_IDENTIFIER
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
		// Skip `let` keyword. The statement parser already checked
		// for it before calling us.
		lex.advance();


		// Create `Fn` node ahead of time so we can insert member data
		// directly instead of copying/moving it into a new node at the end.
		const wpp::node_t node = tree.add<Fn>();


		// Make sure the next token is an identifier, if it is, set the name
		// of our `Fn` node to match.
		if (lex.peek() != TOKEN_IDENTIFIER)
			wpp::error(lex.position(), "function declaration does not have a name.");

		tree.get<Fn>(node).identifier = lex.advance().str();


		// Collect parameters.
		// Advance until we run out of identifiers.
		if (lex.peek() == TOKEN_LPAREN) {
			lex.advance();  // Skip `(`.


			// Check for the first parameter.
			if (lex.peek() == TOKEN_IDENTIFIER)
				tree.get<Fn>(node).parameters.emplace_back(lex.advance().str());

			// While there is a comma, loop until we run out of parameters.
			while (lex.peek() == TOKEN_COMMA) {
				lex.advance(); // Skip `,`.


				// Check if there is a parameter.
				if (lex.peek() != TOKEN_IDENTIFIER)
					wpp::error(lex.position(), "expecting parameter name to follow comma.");


				// Emplace the parameter name into our `Fn` node's list of params.
				tree.get<Fn>(node).parameters.emplace_back(lex.advance().str());
			}


			// Make sure parameter list is terminated by `)`.
			if (lex.advance() != TOKEN_RPAREN)
				wpp::error(lex.position(), "parameter list is unterminated.");
		}

		// Parse the function body.
		const wpp::node_t body = expression(lex, tree);
		tree.get<Fn>(node).body = body;

		return node;
	}


	// Parse a string.
	// `"hey" 'hello' "a\nb\nc\n"`
	inline wpp::node_t string(wpp::Lexer& lex, wpp::AST& tree) {
		const auto delim = lex.advance(wpp::modes::string); // Store delimeter.

		// Create our string node.
		const wpp::node_t node = tree.add<String>();
		auto& [literal] = tree.get<String>(node);

		// Reserve some space, this is kind of arbitrary.
		literal.reserve(1024);

		// Consume tokens until we reach `delim` or EOF.
		while (lex.peek(wpp::modes::string) != delim) {
			if (lex.peek(wpp::modes::string) == TOKEN_EOF)
				wpp::error(lex.position(), "reached EOF while parsing string.");

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

			else if (part == TOKEN_ESCAPE_HEX)
				literal.append("hex");

			else if (part == TOKEN_ESCAPE_BIN)
				literal.append("bin");

			else
				literal.append(part.str());
		}

		lex.advance(wpp::modes::string);

		return node;
	}


	// Parse a function call.
	inline wpp::node_t call(wpp::Lexer& lex, wpp::AST& tree) {
		const wpp::node_t node = tree.add<FnInvoke>(lex.position());

		tree.get<FnInvoke>(node).identifier = lex.advance().str();

		// Optional arguments.
		if (lex.peek() == TOKEN_LPAREN) {
			lex.advance(); // skip lparen.

			if (lex.peek() == TOKEN_RPAREN)
				wpp::error(lex.position(), "empty arguments, drop the parens.");

			// Collect arguments.
			wpp::node_t expr;

			expr = expression(lex, tree);
			tree.get<FnInvoke>(node).arguments.emplace_back(expr);

			while (lex.peek() == TOKEN_COMMA) {
				lex.advance();
				expr = expression(lex, tree);
				tree.get<FnInvoke>(node).arguments.emplace_back(expr);
			}

			if (lex.advance() != TOKEN_RPAREN)
				wpp::error(lex.position(), "no closing parenthesis at end of argument list.");
		}

		return node;
	}


	inline wpp::node_t nspace(wpp::Lexer& lex, wpp::AST& tree) {
		// Skip `namespace` token, we already known it's there because
		// of it being seen by our caller, the statement parser.
		lex.advance();


		// Create `Ns` node.
		const wpp::node_t node = tree.add<Ns>();


		// Expect identifier.
		if (lex.peek() != TOKEN_IDENTIFIER)
			wpp::error(lex.position(), "namespace does not have a name.");

		// Set name of `Ns`.
		tree.get<Ns>(node).identifier = lex.advance().str();


		// Expect opening brace.
		if (lex.advance() != TOKEN_LBRACE)
			wpp::error(lex.position(), "expecting '{' to follow name.");


		// Loop through body of namespace and collect statements.
		if (lex.peek() != TOKEN_RBRACE) {
			// Parse statement and then append it to statements vector in `Ns`.
			// The reason we seperate parsing and emplacing the node ID is
			// to prevent dereferencing an invalidated iterator.
			// If `tree` resizes while parsing the statement, by the time it returns
			// and is emplaced, the reference to the `Ns` node in `tree` might
			// be invalidated.
			do {
				const wpp::node_t stmt = statement(lex, tree);
				tree.get<Ns>(node).statements.emplace_back(stmt);
			} while (peek_is_stmt(lex.peek()));
		}


		// Expect closing brace.
		if (lex.advance() != TOKEN_RBRACE)
			wpp::error(lex.position(), "namespace is unterminated.");


		// Return index to `Ns` node we created at the top of this function.
		return node;
	}


	// Parse a block.
	inline wpp::node_t block(wpp::Lexer& lex, wpp::AST& tree) {
		lex.advance(); // skip lbrace

		const wpp::node_t node = tree.add<Block>();

		// check if theres a statement, otherwise its just a single expression
		if (peek_is_stmt(lex.peek())) {
			do {
				const wpp::node_t stmt = statement(lex, tree);
				tree.get<Block>(node).statements.emplace_back(stmt);
			} while (peek_is_stmt(lex.peek()));
		}

		const wpp::node_t expr = expression(lex, tree);
		tree.get<Block>(node).expr = expr;

		if (peek_is_expr(lex.peek()))
			wpp::error(lex.position(), "block can only have one trailing expression.");

		if (lex.advance() != TOKEN_RBRACE)
			wpp::error(lex.position(), "block is unterminated.");

		return node;
	}


	// Parse an expression.
	inline wpp::node_t expression(wpp::Lexer& lex, wpp::AST& tree) {
		wpp::node_t node;

		const auto lookahead = lex.peek();

		if (lookahead == TOKEN_IDENTIFIER)
			node = wpp::call(lex, tree);

		else if (peek_is_string(lookahead))
			node = wpp::string(lex, tree);

		else if (lookahead == TOKEN_LBRACE)
			node = wpp::block(lex, tree);

		else
			wpp::error(lex.position(), "expecting an expression.");

		if (lex.peek() == TOKEN_CAT) {
			lex.advance();
			node = tree.add<Concat>(node, expression(lex, tree));
		}

		return node;
	}


	// Parse a statement.
	inline wpp::node_t statement(wpp::Lexer& lex, wpp::AST& tree) {
		const auto lookahead = lex.peek();

		// function declaration.
		if (lookahead == TOKEN_LET)
			return wpp::function(lex, tree);

		// namespace
		else if (lookahead == TOKEN_NAMESPACE)
			return wpp::nspace(lex, tree);

		wpp::error(lex.position(), "expecting a statement.");
	}


	// Parse a document.
	// A document is just a series of zero or more expressions.
	inline wpp::node_t document(wpp::Lexer& lex, wpp::AST& tree) {
		const wpp::node_t node = tree.add<Document>();

		// Consume expressions until we encounter eof or an error.
		while (lex.peek() != TOKEN_EOF) {
			wpp::node_t stmt_or_expr;
			const auto lookahead = lex.peek();

			// statement
			if (peek_is_stmt(lookahead))
				stmt_or_expr = statement(lex, tree);

			// expression
			else if (peek_is_expr(lookahead))
				stmt_or_expr = expression(lex, tree);

			else
				wpp::error(lex.position(), "expecting either a statement or an expression.");

			tree.get<Document>(node).exprs_or_stmts.emplace_back(stmt_or_expr);
		}

		return node;
	}
}


#endif
