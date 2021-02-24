#pragma once

#ifndef WOTPP_CHAR
#define WOTPP_CHAR

#include <utility>
#include <cstdint>

// Common character related utilities.

namespace wpp {
	template <typename... Ts>
	constexpr bool in_group(char c, Ts&&... args) {
		return ((c == args) or ...);
	}

	constexpr bool in_range(char c, char lower, char upper) {
		return c >= lower and c <= upper;
	}

	constexpr bool is_digit(char c) {
		return in_range(c, '0', '9');
	}

	constexpr bool is_lower(char c) {
		return in_range(c, 'a', 'z');
	}

	constexpr bool is_upper(char c) {
		return in_range(c, 'A', 'Z');
	}

	constexpr bool is_alpha(char c) {
		return is_lower(c) or is_upper(c);
	}

	constexpr bool is_alphanumeric(char c) {
		return is_alpha(c) or is_digit(c);
	}

	constexpr bool is_whitespace(char c) {
		return in_group(c, ' ', '\n', '\t', '\v', '\f', '\r');
	}

	constexpr bool is_hex(char c) {
		return in_range(c, 'A', 'F') or in_range(c, 'a', 'f') or is_digit(c);
	}

	constexpr bool is_bin(char c) {
		return in_group(c, '0', '1');
	}

	constexpr bool is_identifier(char c) {
		return is_lower(c) or is_upper(c) or is_digit(c) or in_group(c, '_', '.', ':', '/');
	}

	// Get the size of a UTF-8 codepoint.
	inline uint8_t utf_size(const char* ptr) {
		uint8_t out = 0;

		const bool vals[] = {
			(*ptr & 0b10000000) == 0b00000000,
			(*ptr & 0b11100000) == 0b11000000,
			(*ptr & 0b11110000) == 0b11100000,
			(*ptr & 0b11111000) == 0b11110000,
		};

		for (uint8_t i = 0; i < 4; ++i)
			vals[i] && (out = i);

		return out + 1;
	}

	// Convert a hex digit to an int.
	inline uint8_t hex_to_digit(char c) {
		if (c >= '0' && c <= '9')
			return c - '0';

		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;

		if (c >= 'A' && c <= 'F')
			return c - 'A' + 10;

		return '\0';
	}
}

#endif
