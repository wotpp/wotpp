#pragma once

#ifndef WOTPP_COLORS
#define WOTPP_COLORS

namespace wpp {
	constexpr auto ANSI_RESET             = "\x1b[0m";
	constexpr auto ANSI_BOLD              = "\x1b[1m";

	constexpr auto ANSI_FG_BLACK          = "\x1b[30m";
	constexpr auto ANSI_FG_RED            = "\x1b[31m";
	constexpr auto ANSI_FG_GREEN          = "\x1b[32m";
	constexpr auto ANSI_FG_YELLOW         = "\x1b[33m";
	constexpr auto ANSI_FG_BLUE           = "\x1b[34m";
	constexpr auto ANSI_FG_MAGENTA        = "\x1b[35m";
	constexpr auto ANSI_FG_CYAN           = "\x1b[36m";
	constexpr auto ANSI_FG_WHITE          = "\x1b[37m";

	constexpr auto ANSI_FG_BLACK_BRIGHT   = "\x1b[30;1m";
	constexpr auto ANSI_FG_RED_BRIGHT     = "\x1b[31;1m";
	constexpr auto ANSI_FG_GREEN_BRIGHT   = "\x1b[32;1m";
	constexpr auto ANSI_FG_YELLOW_BRIGHT  = "\x1b[33;1m";
	constexpr auto ANSI_FG_BLUE_BRIGHT    = "\x1b[34;1m";
	constexpr auto ANSI_FG_MAGENTA_BRIGHT = "\x1b[35;1m";
	constexpr auto ANSI_FG_CYAN_BRIGHT    = "\x1b[36;1m";
	constexpr auto ANSI_FG_WHITE_BRIGHT   = "\x1b[37;1m";


	constexpr auto ANSI_BG_BLACK          = "\x1b[40m";
	constexpr auto ANSI_BG_RED            = "\x1b[41m";
	constexpr auto ANSI_BG_GREEN          = "\x1b[42m";
	constexpr auto ANSI_BG_YELLOW         = "\x1b[43m";
	constexpr auto ANSI_BG_BLUE           = "\x1b[44m";
	constexpr auto ANSI_BG_MAGENTA        = "\x1b[45m";
	constexpr auto ANSI_BG_CYAN           = "\x1b[46m";
	constexpr auto ANSI_BG_WHITE          = "\x1b[47m";


	constexpr auto ANSI_BG_BLACK_BRIGHT   = "\x1b[40;1m";
	constexpr auto ANSI_BG_RED_BRIGHT     = "\x1b[41;1m";
	constexpr auto ANSI_BG_GREEN_BRIGHT   = "\x1b[42;1m";
	constexpr auto ANSI_BG_YELLOW_BRIGHT  = "\x1b[43;1m";
	constexpr auto ANSI_BG_BLUE_BRIGHT    = "\x1b[44;1m";
	constexpr auto ANSI_BG_MAGENTA_BRIGHT = "\x1b[45;1m";
	constexpr auto ANSI_BG_CYAN_BRIGHT    = "\x1b[46;1m";
	constexpr auto ANSI_BG_WHITE_BRIGHT   = "\x1b[47;1m";
}

#endif
