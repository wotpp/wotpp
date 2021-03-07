#pragma once

#ifndef WOTPP_CHAR
#define WOTPP_CHAR

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
		return c == ' ' or in_range(c, '\t', '\r');
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
