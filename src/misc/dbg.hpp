#pragma once

#ifndef WOTPP_DBG
#define WOTPP_DBG

#include <iostream>
#include <filesystem>
#include <utility>

#include <misc/colours.hpp>

	#ifndef NDEBUG
		#define DBG(...) \
			do { \
				[dbg_fn_name__ = __func__] (auto&&... dbg_args__) { \
					auto& os = [&] () -> std::ostream& { \
						return ( \
							std::cerr << ANSI_FG_MAGENTA << "log " << ANSI_RESET << \
							std::filesystem::path(__FILE__).filename().string() << ":" << \
							__LINE__ << " in `" << ANSI_FG_YELLOW << dbg_fn_name__ << ANSI_RESET << "`"); \
					} (); \
					if constexpr(sizeof...(dbg_args__) > 0) { \
						os << " => " << ANSI_BOLD;\
						(os << ... << std::forward<decltype(dbg_args__)>(dbg_args__)) << ANSI_RESET << '\n'; \
					} else { \
						os << ANSI_RESET << '\n'; \
					} \
				} ( __VA_ARGS__ ); \
			} while (false)

	#else
		#define DBG(...) do {} while (false);
	#endif

#endif
