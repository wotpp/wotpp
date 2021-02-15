#pragma once

#ifndef WOTPP_TOKEN
#define WOTPP_TOKEN

#include <string>
#include <iostream>
#include <cstdint>

#include <structures/view.hpp>

// Token structure that combines a type with a view of it's contents.

namespace wpp {
	using token_type_t = uint8_t;

	struct Token {
		View view{};
		wpp::token_type_t type = 0;


		constexpr Token() {}

		constexpr Token(View view_, token_type_t type_):
			view(view_), type(type_) {}


		friend bool operator==(const Token& tok, token_type_t t) {
			return tok.type == t;
		}

		friend bool operator!=(const Token& tok, token_type_t t) {
			return not(tok == t);
		}

		friend bool operator==(token_type_t t, const Token& tok) {
			return tok == t;
		}

		friend bool operator!=(token_type_t t, const Token& tok) {
			return not(tok == t);
		}

		friend bool operator==(const Token& tok, const View& v) {
			return tok.view == v;
		}

		friend bool operator!=(const Token& tok, const View& v) {
			return not(tok == v);
		}

		friend bool operator==(const View& v, const Token& tok) {
			return tok.view == v;
		}

		friend bool operator!=(const View& v, const Token& tok) {
			return not(tok == v);
		}


		bool operator==(const Token& t) const {
			return type == t and view == t;
		}

		bool operator!=(const Token& t) const {
			return not(*this == t);
		}


		std::string str() const {
			return view.str();
		}
	};

	inline std::ostream& operator<<(std::ostream& os, const Token& t) {
		const auto& [view, type] = t;
		return (os << view);
	}
}

#endif
