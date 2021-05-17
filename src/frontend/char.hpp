#pragma once

#ifndef WOTPP_CHAR
#define WOTPP_CHAR

#include <algorithm>
#include <cstdint>

#include <misc/utf8.hpp>

// Common character related utilities.

namespace wpp {
	// Convert a hex digit to an int.
	inline uint8_t hex_to_int_digit(char c) {
		if (c >= '0' && c <= '9')
			return c - '0';

		else if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;

		else if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;

		return 0;
	}

	inline uint8_t dec_to_int_digit(char c) {
		return c - '0';
	}

	inline int dec_to_int_view(const View& v) {
		int n = 0;
		int mul = 1;

		auto ptr = v.ptr;

		if (*ptr == '-') {
			mul = -1;
			++ptr;
		}

		for (; ptr != v.ptr + v.length; ++ptr)
			n = (n * 10) + dec_to_int_digit(*ptr);

		return n * mul;
	}


	template <typename... Ts>
	constexpr bool in_group(const char* c, Ts&&... args) {
		return ((*c == args) or ...);
	}

	constexpr bool in_range(const char* c, char lower, char upper) {
		return *c >= lower and *c <= upper;
	}

	constexpr bool is_digit(const char* c) {
		return in_range(c, '0', '9');
	}

	constexpr bool is_lower(const char* c) {
		return in_range(c, 'a', 'z');
	}

	constexpr bool is_upper(const char* c) {
		return in_range(c, 'A', 'Z');
	}

	constexpr bool is_alpha(const char* c) {
		return is_lower(c) or is_upper(c);
	}

	constexpr bool is_alphanumeric(const char* c) {
		return is_alpha(c) or is_digit(c);
	}

	constexpr bool is_grouping(const char* c) {
		return in_group(c, '[', ']', '(', ')', '{', '}');
	}

	constexpr bool is_whitespace(const char* c) {
		const auto is_whitespace_utf8 = [&c] () {
			int32_t chr = utf8::decode(c);

			return
				chr == 0x0085 or
				chr == 0x00A0 or
				chr == 0x1680 or
				(chr >= 0x2000 and chr <= 0x200A) or
				chr == 0x2028 or
				chr == 0x2029 or
				chr == 0x202F or
				chr == 0x205F or
				chr == 0x3000
			;
		};

		return *c == ' ' or in_range(c, '\t', '\r') or is_whitespace_utf8();
	}

	constexpr bool is_hex(const char* c) {
		return in_range(c, 'A', 'F') or in_range(c, 'a', 'f') or is_digit(c);
	}

	constexpr bool is_bin(const char* c) {
		return in_group(c, '0', '1');
	}

	constexpr bool is_quote(const char* c) {
		return in_group(c, '"', '\'');
	}

	constexpr bool is_escape(const char* c) {
		return in_group(c, '\\', 'n', 'r', 't', 'b', 'x') or is_quote(c);
	}

	constexpr bool is_eof(const char* c) {
		return *c == '\0';
	}

	constexpr bool is_newline(const char* c) {
		return
			*c == '\n' or
			(*c == '\r' and *(c + 1) == '\n')
		;
	}


	// Collapse consecutive runs of characters.
	inline std::string collapse_repeated(std::string&& str) {
		char* const begin = str.data();
		char* const end = str.data() + str.size();

		// Check if adjacent characters are identical.
		const auto pred = [] (const char* lhs, const char* rhs) {
			return utf8::decode(lhs) == utf8::decode(rhs);
		};

		char* first = begin;
		char* result = begin;

		// std::unique: https://en.cppreference.com/w/cpp/algorithm/unique#Possible_implementation
		while (++first != end) {
			if (not pred(result, first) and ++result != first)
				*result = *first;
		}

		++result;

		// Erase all but one of the characters.
		str.erase(str.begin() + (result - begin), str.end());

		return std::move(str);
	}
}

#endif
