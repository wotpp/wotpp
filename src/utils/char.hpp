#pragma once

#ifndef WOTPP_CHAR
#define WOTPP_CHAR

#include <utility>

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
		return in_range(c, 'A', 'F') or in_range(c, 'a', 'f') or is_digit(c) or c == '_';
	}

	constexpr bool is_bin(char c) {
		return in_group(c, '0', '1', '_');
	}

	constexpr bool is_identifier(char c) {
		return is_lower(c) or is_upper(c) or is_digit(c) or in_group(c, '_', '.', ':', '/');
	}

	// Increment pointer as long as predicate is satisfied.
	template <typename Pred, typename... Ts>
	constexpr auto consume(const char*& ptr, const char* const begin, const Pred pred, Ts&&... args) {
		do {
			++ptr;
		} while (pred(*ptr, std::forward<Ts>(args)...));

		return ptr - begin;
	}
}

#endif
