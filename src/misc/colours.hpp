#pragma once

#ifndef WOTPP_COLORS
#define WOTPP_COLORS

#if !defined(WPP_DISABLE_COLOUR)
	#define ANSI_RESET              "\x1b[0m"
	#define ANSI_BOLD               "\x1b[1m"

	#define ANSI_FG_BLACK           "\x1b[30m"
	#define ANSI_FG_RED             "\x1b[31m"
	#define ANSI_FG_GREEN           "\x1b[32m"
	#define ANSI_FG_YELLOW          "\x1b[33m"
	#define ANSI_FG_BLUE            "\x1b[34m"
	#define ANSI_FG_MAGENTA         "\x1b[35m"
	#define ANSI_FG_CYAN            "\x1b[36m"
	#define ANSI_FG_WHITE           "\x1b[37m"

	#define ANSI_FG_BLACK_BRIGHT    "\x1b[30;1m"
	#define ANSI_FG_RED_BRIGHT      "\x1b[31;1m"
	#define ANSI_FG_GREEN_BRIGHT    "\x1b[32;1m"
	#define ANSI_FG_YELLOW_BRIGHT   "\x1b[33;1m"
	#define ANSI_FG_BLUE_BRIGHT     "\x1b[34;1m"
	#define ANSI_FG_MAGENTA_BRIGHT  "\x1b[35;1m"
	#define ANSI_FG_CYAN_BRIGHT     "\x1b[36;1m"
	#define ANSI_FG_WHITE_BRIGHT    "\x1b[37;1m"


	#define ANSI_BG_BLACK           "\x1b[40m"
	#define ANSI_BG_RED             "\x1b[41m"
	#define ANSI_BG_GREEN           "\x1b[42m"
	#define ANSI_BG_YELLOW          "\x1b[43m"
	#define ANSI_BG_BLUE            "\x1b[44m"
	#define ANSI_BG_MAGENTA         "\x1b[45m"
	#define ANSI_BG_CYAN            "\x1b[46m"
	#define ANSI_BG_WHITE           "\x1b[47m"


	#define ANSI_BG_BLACK_BRIGHT    "\x1b[40;1m"
	#define ANSI_BG_RED_BRIGHT      "\x1b[41;1m"
	#define ANSI_BG_GREEN_BRIGHT    "\x1b[42;1m"
	#define ANSI_BG_YELLOW_BRIGHT   "\x1b[43;1m"
	#define ANSI_BG_BLUE_BRIGHT     "\x1b[44;1m"
	#define ANSI_BG_MAGENTA_BRIGHT  "\x1b[45;1m"
	#define ANSI_BG_CYAN_BRIGHT     "\x1b[46;1m"
	#define ANSI_BG_WHITE_BRIGHT    "\x1b[47;1m"

#else
	#define ANSI_RESET              ""
	#define ANSI_BOLD               ""

	#define ANSI_FG_BLACK           ""
	#define ANSI_FG_RED             ""
	#define ANSI_FG_GREEN           ""
	#define ANSI_FG_YELLOW          ""
	#define ANSI_FG_BLUE            ""
	#define ANSI_FG_MAGENTA         ""
	#define ANSI_FG_CYAN            ""
	#define ANSI_FG_WHITE           ""

	#define ANSI_FG_BLACK_BRIGHT    ""
	#define ANSI_FG_RED_BRIGHT      ""
	#define ANSI_FG_GREEN_BRIGHT    ""
	#define ANSI_FG_YELLOW_BRIGHT   ""
	#define ANSI_FG_BLUE_BRIGHT     ""
	#define ANSI_FG_MAGENTA_BRIGHT  ""
	#define ANSI_FG_CYAN_BRIGHT     ""
	#define ANSI_FG_WHITE_BRIGHT    ""


	#define ANSI_BG_BLACK           ""
	#define ANSI_BG_RED             ""
	#define ANSI_BG_GREEN           ""
	#define ANSI_BG_YELLOW          ""
	#define ANSI_BG_BLUE            ""
	#define ANSI_BG_MAGENTA         ""
	#define ANSI_BG_CYAN            ""
	#define ANSI_BG_WHITE           ""


	#define ANSI_BG_BLACK_BRIGHT    ""
	#define ANSI_BG_RED_BRIGHT      ""
	#define ANSI_BG_GREEN_BRIGHT    ""
	#define ANSI_BG_YELLOW_BRIGHT   ""
	#define ANSI_BG_BLUE_BRIGHT     ""
	#define ANSI_BG_MAGENTA_BRIGHT  ""
	#define ANSI_BG_CYAN_BRIGHT     ""
	#define ANSI_BG_WHITE_BRIGHT    ""
#endif

#endif
