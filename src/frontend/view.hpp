#pragma once

#ifndef WOTPP_VIEW
#define WOTPP_VIEW

#include <string>
#include <cstring>
#include <cstdint>


namespace wpp {
	struct View {
		const char *ptr = nullptr;
		uint32_t length = 0;


		constexpr View() {}

		constexpr View(const char* const ptr_, const char* const end_):
			ptr(ptr_), length(end_ - ptr_) {}

		constexpr View(const char* const ptr_, int length_):
			ptr(ptr_), length(length_) {}


		constexpr char at(int n) const {
			return *(ptr + n);
		}


		// Comparison operators.
		// First compares length and then failing that, character by character.
		friend bool operator==(const View& v, const char* const s) {
			return
				static_cast<uint32_t>(std::strlen(s)) == v.length and
				std::strncmp(v.ptr, s, v.length) == 0
			;
		}

		friend bool operator==(const View& v, char c) {
			return *v.ptr == c;
		}

		friend bool operator!=(const View& v, const char* const s) {
			return not(v == s);
		}

		friend bool operator!=(const View& v, char c) {
			return not(v == c);
		}


		friend bool operator==(const char* const s, const View& v) {
			return v == s;
		}

		friend bool operator!=(const char* const s, const View& v) {
			return not(v == s);
		}

		friend bool operator==(char c, const View& v) {
			return v == c;
		}

		friend bool operator!=(char c, const View& v) {
			return not(v == c);
		}

		bool operator==(const View& v) const {
			return
				v.length == length and
				std::strncmp(ptr, v.ptr, length) == 0
			;
		}

		bool operator!=(const View& v) const {
			return not(*this == v);
		}


		std::string str() const {
			return std::string{ ptr, static_cast<std::string::size_type>(length) };
		}
	};
}

#endif
