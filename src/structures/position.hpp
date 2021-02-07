#pragma once

#ifndef WOTPP_POSITION
#define WOTPP_POSITION

#include <iostream>

// Track a position in a source file.
// Calculated as needed when an error occurs.

namespace wpp {
	struct Position {
		int line = 1, column = 1;
	};


	std::ostream& operator<<(std::ostream& os, const Position& pos) {
		return (os << pos.line << ':' << pos.column);
	}


	Position position(const char* ptr, const char* const end) {
		Position coord;
		auto& [line, column] = coord;

		while (ptr++ != end) {
			if (*ptr == '\n') {
				column = 1;
				line++;
			}

			else {
				column++;
			}
		}

		if (column > 1)
			column--;

		return coord;
	}
}

#endif
