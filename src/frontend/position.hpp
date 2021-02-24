#pragma once

#ifndef WOTPP_POSITION
#define WOTPP_POSITION

#include <iostream>
#include <string>

// Track a position in a source file.
// Calculated as needed when an error occurs.

namespace wpp {
	struct Position {
		std::string path;
		const char* msg = nullptr;
		int line = 1, column = 1;
	};


	inline std::ostream& operator<<(std::ostream& os, const Position& pos) {
		const auto& [fname, msg, line, column] = pos;

		if (msg)
			return (os << msg);

		else
			return (os << fname << ':' << line << ':' << column);
	}


	inline Position position(
		const std::string& fname,
		const char* ptr,
		const char* const end,
		int line_offset = 0,
		int column_offset = 0
	) {
		Position coord;
		auto& [path, msg, line, column] = coord;

		path = fname;

		while (ptr != end) {
			if (*ptr == '\n') {
				column = 1;
				line++;
			}

			else {
				column++;
			}

			++ptr;
		}

		if (*ptr == '\0') {
			msg = "EOF";
		}

		line += line_offset;
		column += column_offset;

		return coord;
	}
}

#endif
