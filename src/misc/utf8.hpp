#pragma once

#ifndef WOTPP_UTF8
#define WOTPP_UTF8

namespace wpp::utf8 {
	// Get the size of a UTF-8 codepoint.
	inline uint8_t codepoint_size(const char* const ptr) {
		// if      ((*ptr & 0b10000000) == 0b00000000) return 1;
		if ((*ptr & 0b11100000) == 0b11000000) return 2;
		else if ((*ptr & 0b11110000) == 0b11100000) return 3;
		else if ((*ptr & 0b11111000) == 0b11110000) return 4;

		return 1;
	}


	inline int string_size(const char* ptr) {
		int n = 0;

		while (*ptr++) {
			int sz = codepoint_size(ptr);
			n += sz;
			ptr += sz;
		}

		return n;
	}


	// Find the beginning of the first codepoint iterating backwards
	// from a given pointer.
	inline const char* prev(const char* ptr) {
		--ptr;

		// Make sure byte is of the form `10xx_xxxx`.
		while ((*ptr & 0b10000000) and not (*ptr & 0b01000000))
			--ptr;

		return ptr;
	}


	inline const char* next(const char* ptr) {
		return ptr + codepoint_size(ptr);
	}


	inline int decode(const char* const c) {
		int out = *c;

		switch (codepoint_size(c)) {
			case 1: return out;
			case 2: return ((out & 31) << 6)  |  (c[1] & 63);
			case 3: return ((out & 15) << 12) | ((c[1] & 63) << 6)  |  (c[2] & 63);
			case 4: return ((out & 7)  << 18) | ((c[1] & 63) << 12) | ((c[2] & 63) << 6) | (c[3] & 63);
		}

		return 0;
	}

	namespace detail {
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
	}

	inline bool validate(const char*& str) {
		uint32_t codepoint = 0;
		uint32_t state = 0;

		const uint8_t* s = (const uint8_t*)str;

		for (; *s; ) {
			str = (const char*)s;
			state = detail::decode(state, codepoint, *s);

			if (state == detail::UTF8_REJECT)
				break;

			else
				++s;
		}

		return state == detail::UTF8_ACCEPT;
	}
}

#endif
