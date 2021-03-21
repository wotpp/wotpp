#pragma once

#ifndef WOTPP_CHAR
#define WOTPP_CHAR

#include <algorithm>
#include <cstdint>

// Common character related utilities.

namespace wpp {
	// Get the size of a UTF-8 codepoint.
	inline uint8_t size_utf8(const char* ptr) {
		if      ((*ptr & 0b10000000) == 0b00000000) return 1;
		else if ((*ptr & 0b11100000) == 0b11000000) return 2;
		else if ((*ptr & 0b11110000) == 0b11100000) return 3;
		else if ((*ptr & 0b11111000) == 0b11110000) return 4;
		return 0;
	}


	// Find the beginning of the first codepoint iterating backwards
	// from a given pointer.
	inline const char* prev_char_utf8(const char* const begin, const char* ptr) {
		do {
			--ptr;
		} while (
			ptr >= begin and
			// Make sure byte is of the form `10xx_xxxx`.
			((*ptr & 0b10000000) and not (*ptr & 0b01000000))
		);

		return ptr;
	}


	inline int decode_utf8(const char* const c) {
		int out = *c;

		switch (wpp::size_utf8(c)) {
			case 1: return out;
			case 2: return ((out & 31) << 6)  |  (c[1] & 63);
			case 3: return ((out & 15) << 12) | ((c[1] & 63) << 6)  |  (c[2] & 63);
			case 4: return ((out & 7)  << 18) | ((c[1] & 63) << 12) | ((c[2] & 63) << 6) | (c[3] & 63);
		}

		return 0;
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

	inline bool is_whitespace_utf8(int32_t c) {
		return
			c == 0x0085 or
			c == 0x00A0 or
			c == 0x1680 or
			(c >= 0x2000 and c <= 0x200A) or
			c == 0x2028 or
			c == 0x2029 or
			c == 0x202F or
			c == 0x205F or
			c == 0x3000
		;
	}

	constexpr bool is_whitespace(const char* c) {
		return *c == ' ' or in_range(c, '\t', '\r') or is_whitespace_utf8(decode_utf8(c));
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

	constexpr bool is_identifier(const char* c) {
		return is_lower(c) or is_upper(c) or is_digit(c) or in_group(c, '_', '.', ':', '/');
	}


	// Collapse consecutive whitespace.
	inline void collapse_whitespace(std::string& str) {
		// Replace all whitespace with just ' '.
		char* const begin = str.data();
		char* const end = str.data() + str.size();

		for (char* ptr = begin; ptr != end; ptr += wpp::size_utf8(ptr)) {
			if (wpp::is_whitespace(ptr))
				*ptr = ' ';
		}

		// Collapse repeated spaces.
		str.erase(std::unique(str.begin(), str.end(), [] (char lhs, char rhs) {
			return lhs == rhs and lhs == ' ';
		}), str.end());

		// Strip leading and trailing whitespace.
		if (str.front() == ' ')
			str.erase(str.begin(), str.begin() + 1);

		if (str.back() == ' ')
			str.erase(str.end() - 1, str.end());
	}


	// Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
	// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

	constexpr int UTF8_ACCEPT = 0;
	constexpr int UTF8_REJECT = 1;

	constexpr uint8_t utf8d[] = {
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
		8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
		0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
		0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
		0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
		1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
		1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
		1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
		1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
	};

	inline uint32_t decode(uint32_t& state, uint32_t& codep, uint32_t b) {
		uint32_t type = utf8d[b];

		if (state != UTF8_ACCEPT)
			codep = (b & 0x3fu) | (codep << 6);

		else
			codep = (0xff >> type) & b;

		return utf8d[256 + state * 16 + type];
	}

	inline bool validate_utf8(const char*& str) {
		uint32_t codepoint = 0;
		uint32_t state = 0;

		const uint8_t* s = (const uint8_t*)str;

		for (; *s; ) {
			str = (const char*)s;
			state = decode(state, codepoint, *s);

			if (state == UTF8_REJECT)
				break;

			else
				++s;
		}

		return state == UTF8_ACCEPT;
	}
}

#endif
