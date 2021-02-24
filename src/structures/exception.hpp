#pragma once

#ifndef WOTPP_EXCEPTION
#define WOTPP_EXCEPTION

#include <stdexcept>
#include <string>

#include <frontend/position.hpp>
#include <misc/util/util.hpp>

namespace wpp {
	constexpr const char* DEFAULT_ERROR_MESSAGE = "an error has occurred.";

	struct Exception: public std::runtime_error {
		wpp::Position pos;

		template <typename... Ts>
		Exception(const wpp::Position& pos_, Ts&&... args):
			std::runtime_error(wpp::cat(args...)),
			pos(pos_) {}

		Exception(const wpp::Position& pos_):
			std::runtime_error(DEFAULT_ERROR_MESSAGE),
			pos(pos_) {}

		Exception():
			std::runtime_error(DEFAULT_ERROR_MESSAGE),
			pos() {}
	};
}

#endif
