#pragma once

#ifndef WOTPP_GENPROG
#define WOTPP_GENPROG

#include <string>

#include <structures/rng.hpp>
#include <tinge.hpp>

// Generate a random but syntactically valid program for fuzz testing
// and benchmarking.

namespace wpp {
	constexpr int MAX_DEPTH = 10;

	inline std::string gen_identifier(wpp::Random&);
	inline char gen_char(wpp::Random&);
	inline char gen_digit(wpp::Random&);
	inline char gen_whitespace(wpp::Random&);

	inline std::string gen_document(wpp::Random&, int, int);
	inline std::string gen_expression(wpp::Random&, int, int);
	inline std::string gen_statement(wpp::Random&, int, int);
	inline std::string gen_call(wpp::Random&, int, int);
	inline std::string gen_block(wpp::Random&, int, int);
	inline std::string gen_cat(wpp::Random&, int, int);
	inline std::string gen_string(wpp::Random&, int, int);
	inline std::string gen_comment(wpp::Random&, int, int);
	inline std::string gen_func(wpp::Random&, int, int);
	inline std::string gen_nspace(wpp::Random&, int, int);


	inline char gen_char(wpp::Random& rng) {
		switch (rng.range(0, 1)) {
			case 0: return rng.range('a', 'z');
			case 1: return rng.range('A', 'Z');
		}

		return 'a';
	}


	inline char gen_whitespace(wpp::Random& rng) {
		switch (rng.range(0, 2)) {
			case 0: return '\n';
			case 1: return '\t';
			case 2: return ' ';
		}

		return ' ';
	}


	inline char gen_digit(wpp::Random& rng) {
		return rng.range('0', '9');
	}


	inline std::string gen_identifier(wpp::Random& rng) {
		auto length = rng.range(0, 5);

		std::string out;
		out += gen_char(rng);

		while (length--) {
			switch (rng.range(0, 1)) {
				case 0: out += gen_char(rng); break;
				case 1: out += gen_digit(rng); break;
				// case 2: out += '_'; break;
			}
		}

		return out;
		// return "foo";
	}


	inline std::string gen_document(wpp::Random& rng, int indent = 0, int depth = 0) {
		std::string str;

		for (int i = 0; i < (int)rng.range(10, 50); i++) {
			switch (rng.range(0, 1)) {
				case 0: str += gen_expression(rng, indent, depth + 1);
				case 1: str += gen_statement(rng, indent, depth + 1);
			}

			str += '\n';
		}

		return str;
	}


	inline std::string gen_expression(wpp::Random& rng, int indent = 0, int depth = 0) {
		if (depth >= MAX_DEPTH)
			return gen_string(rng, indent, depth + 1);

		switch (rng.range(0, 3)) {
			case 0: return gen_call(rng, indent, depth + 1) + '\n';
			case 1: return gen_block(rng, indent, depth + 1) + '\n';
			case 2: return gen_cat(rng, indent, depth + 1) + '\n';
			case 3: return gen_string(rng, indent, depth + 1);
		}

		return "";
	}


	inline std::string gen_statement(wpp::Random& rng, int indent = 0, int depth = 0) {
		if (depth >= MAX_DEPTH)
			return gen_comment(rng, indent, depth + 1);

		switch (rng.range(0, 2)) {
			case 0: return gen_func(rng, indent, depth + 1) + '\n';
			case 1: return gen_nspace(rng, indent, depth + 1) + '\n';
			case 2: return gen_comment(rng, indent, depth + 1);
		}

		return "";
	}


	inline std::string gen_call(wpp::Random& rng, int indent = 0, int depth = 0) {
		std::string str;
		str += tinge::space(indent) + gen_identifier(rng);

		// optional args
		if (rng.boolean()) {
			str += "(\n";

			for (int i = 0; i < (int)rng.range(0, 6); ++i) {
				str += gen_expression(rng, indent + 1, depth + 1);
				str += ", \n";
			}

			str += gen_expression(rng, indent + 1, depth + 1);

			str += "\n" + tinge::space(indent) + ")";
		}

		// str += "\n";

		return str;
	}


	inline std::string gen_block(wpp::Random& rng, int indent = 0, int depth = 0) {
		std::string str;
		str += tinge::space(indent) + "{\n";

		for (int i = 0; i < (int)rng.range(0, 4); ++i) {
			str += gen_statement(rng, indent + 1, depth + 1);
			str += "\n";
		}

		str += gen_expression(rng, indent + 1, depth + 1);

		str += "\n" + tinge::space(indent) + "}";

		return str;
	}


	inline std::string gen_cat(wpp::Random& rng, int indent = 0, int depth = 0) {
		std::string str;
		str += gen_expression(rng, indent, depth + 1);
		str += "..\n";
		str += gen_expression(rng, indent + 1, depth + 1);
		return str;
	}


	inline std::string gen_string(wpp::Random& rng, int indent = 0, int = 0) {
		std::string str;
		str += tinge::space(indent) + '"';

		for (int i = 0; i < (int)rng.range(0, 9); ++i) {
			str += gen_identifier(rng);
			str += " ";
		}
		str += gen_identifier(rng);

		str += "\" ";
		return str;
	}


	inline std::string gen_comment(wpp::Random& rng, int indent = 0, int = 0) {
		std::string str;
		str += tinge::space(indent) + "#[";

		for (int i = 0; i < (int)rng.range(0, 9); ++i) {
			str += gen_identifier(rng);
			str += " ";
		}
		str += gen_identifier(rng);

		str += "] ";
		return str;
	}


	inline std::string gen_func(wpp::Random& rng, int indent = 0, int depth = 0) {
		std::string str;
		str += tinge::space(indent) + "let ";

		str += gen_identifier(rng);

		str += '(';

		for (int i = 0; i < (int)rng.range(0, 7); ++i) {
			str += gen_identifier(rng);
			str += ", ";
		}

		str += gen_identifier(rng);

		str += ")\n";

		str += gen_expression(rng, indent + 1, depth + 1);

		// str += "\n";
		return str;
	}


	inline std::string gen_nspace(wpp::Random& rng, int indent = 0, int depth = 0) {
		std::string str;
		str += tinge::space(indent) + "namespace ";
		str += gen_identifier(rng);
		str += " {\n";

		for (int i = 0; i < (int)rng.range(2, 15); ++i) {
			// str += tinge::space(indent);
			str += gen_statement(rng, indent + 2, depth + 1);
			str += "\n";
		}
		str += gen_statement(rng, indent + 2, depth + 1);

		str += tinge::space(indent) + "}";
		return str;
	}
}

#endif

