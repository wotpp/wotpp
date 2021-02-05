#pragma once

#ifndef WOTPP_VIEW
#define WOTPP_VIEW

#include <string>
#include <iostream>
#include <cstring>
#include <cstdint>

#include <utils/util.hpp>

// A view is a non-owning way to view a string.

namespace wpp {
	struct View {
		const char *ptr = nullptr;
		uint32_t length = 0;


		constexpr View() {}

		constexpr View(const char* const ptr_, const char* const end_):
			ptr(ptr_), length(end_ - ptr_) {}

		constexpr View(const char* const ptr_, int length_):
			ptr(ptr_), length(length_) {}


		// Comparison operators.
		// First compares length and then failing that, character by character.
		friend bool operator==(const View& v, const char* const s) {
			return
				static_cast<uint32_t>(std::strlen(s)) == v.length and
				std::strncmp(v.ptr, s, v.length) == 0
			;
		}

		friend bool operator!=(const View& v, const char* const s) {
			return not(v == s);
		}


		friend bool operator==(const char* const s, const View& v) {
			return v == s;
		}

		friend bool operator!=(const char* const s, const View& v) {
			return not(v == s);
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

	inline std::ostream& operator<<(std::ostream& os, const View& v) {
		const auto& [vptr, vlength] = v;
		os.write(vptr, vlength);
		return os;
	}

	// Hashing function for `View` so that we can insert
	// it into unordered_map.
	struct ViewHasher {
		constexpr std::size_t operator()(const View& v) const {
			return wpp::hash_bytes(v.ptr, v.ptr + v.length);
		}
	};
}

#endif
