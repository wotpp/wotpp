#pragma once

#ifndef WOTPP_POSITION
#define WOTPP_POSITION

#include <iostream>
#include <filesystem>
#include <string>

// Track a position in a source file.
// Calculated as needed when an error occurs.

namespace wpp {
	struct Context {
		const std::filesystem::path root{};
		std::filesystem::path file{};

		std::string msg{};

		const char* base = nullptr;
	};


	struct Pos {
		wpp::Context ctx;
		const char* const ptr;
	};

	inline std::ostream& operator<<(std::ostream& os, const wpp::Pos& pos) {
		const auto& [ctx, offset] = pos;
		const auto& [root, file, msg, base] = ctx;

		const char* ptr = base;

		int line = 1, column = 1;

		while (ptr != offset) {
			if (*ptr == '\n') {
				column = 1;
				line++;
			}

			else
				column++;

			++ptr;
		}
		os << "[" << msg << "] ";

		os << std::filesystem::relative(file, root).string() << ":";

		if (*ptr == '\0')
			os << "EOF: ";

		else
			os << line << ":" << column;

		return os;
	}
}

#endif
