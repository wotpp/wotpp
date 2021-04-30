#pragma once

#ifndef WOTPP_COLORS
#define WOTPP_COLORS

namespace wpp {
	enum {
		ANSI_RESET,
		ANSI_BOLD,

		ANSI_FG_BLACK,
		ANSI_FG_RED,
		ANSI_FG_GREEN,
		ANSI_FG_YELLOW,
		ANSI_FG_BLUE,
		ANSI_FG_MAGENTA,
		ANSI_FG_CYAN,
		ANSI_FG_WHITE,

		ANSI_FG_BLACK_BRIGHT,
		ANSI_FG_RED_BRIGHT,
		ANSI_FG_GREEN_BRIGHT,
		ANSI_FG_YELLOW_BRIGHT,
		ANSI_FG_BLUE_BRIGHT,
		ANSI_FG_MAGENTA_BRIGHT,
		ANSI_FG_CYAN_BRIGHT,
		ANSI_FG_WHITE_BRIGHT,

		ANSI_BG_BLACK,
		ANSI_BG_RED,
		ANSI_BG_GREEN,
		ANSI_BG_YELLOW,
		ANSI_BG_BLUE,
		ANSI_BG_MAGENTA,
		ANSI_BG_CYAN,
		ANSI_BG_WHITE,

		ANSI_BG_BLACK_BRIGHT,
		ANSI_BG_RED_BRIGHT,
		ANSI_BG_GREEN_BRIGHT,
		ANSI_BG_YELLOW_BRIGHT,
		ANSI_BG_BLUE_BRIGHT,
		ANSI_BG_MAGENTA_BRIGHT,
		ANSI_BG_CYAN_BRIGHT,
		ANSI_BG_WHITE_BRIGHT,

		ANSI_TOTAL,
	};

	constexpr const char* COLOURS[] = {
		"\x1b[0m", "",
		"\x1b[1m", "",

		"\x1b[30m", "",
		"\x1b[31m", "",
		"\x1b[32m", "",
		"\x1b[33m", "",
		"\x1b[34m", "",
		"\x1b[35m", "",
		"\x1b[36m", "",
		"\x1b[37m", "",

		"\x1b[30;1m", "",
		"\x1b[31;1m", "",
		"\x1b[32;1m", "",
		"\x1b[33;1m", "",
		"\x1b[34;1m", "",
		"\x1b[35;1m", "",
		"\x1b[36;1m", "",
		"\x1b[37;1m", "",

		"\x1b[40m", "",
		"\x1b[41m", "",
		"\x1b[42m", "",
		"\x1b[43m", "",
		"\x1b[44m", "",
		"\x1b[45m", "",
		"\x1b[46m", "",
		"\x1b[47m", "",

		"\x1b[40;1m", "",
		"\x1b[41;1m", "",
		"\x1b[42;1m", "",
		"\x1b[43;1m", "",
		"\x1b[44;1m", "",
		"\x1b[45;1m", "",
		"\x1b[46;1m", "",
		"\x1b[47;1m", "",
	};


	namespace detail {
		inline auto lookup_colour_enabled(int colour) {
			return COLOURS[colour * 2];
		}

		inline auto lookup_colour_disabled(int colour) {
			return COLOURS[colour * 2 + 1];
		}
	}
}

#endif
