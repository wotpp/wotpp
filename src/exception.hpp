#pragma once

#ifndef WOTPP_EXCEPTION
#define WOTPP_EXCEPTION

#include <stdexcept>
#include <vector>
#include <string>

#include <utils/util.hpp>

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

	// This is ugly. TODO: Separate header/implementation files
	struct FnInvoke;
	using Backtrace = std::vector<wpp::FnInvoke>;

	struct BacktraceException: Exception {
		wpp::Backtrace backtrace;

		template <typename... Ts>
		BacktraceException(const wpp::Backtrace& bt_, const wpp::Position& pos_, Ts&&... args):
			Exception(pos_, args...), backtrace(bt_) {}

		BacktraceException(const wpp::Backtrace& bt_, const wpp::Position& pos_):
			Exception(pos_), backtrace(bt_) {}

		BacktraceException(const wpp::Backtrace& bt_):
			Exception(), backtrace(bt_) {}
	};
}

#endif
