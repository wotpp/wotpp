#pragma once

#ifndef WOTPP_ERROR
#define WOTPP_ERROR

#include <string>
#include <iostream>

#include <cstdint>

#include <misc/util/util.hpp>

namespace wpp {
	// Error type.
	// Variadic constructor allows passing any number of objects
	// with operator<< overload. This allows it to act like std::cout or
	// std::cerr except it stores the ouput as a string for later output
	// after the error has been propogated.
	struct Error {
		uint32_t code = 0;
		std::string msg;


		Error() {}

		template <typename... Ts>
		Error(uint32_t code_, Ts&&... args):
			code(code_),
			msg(wpp::cat(args...)) {}

		Error(uint32_t code_, const std::string& msg_):
			code(code_),
			msg(msg_) {}


		operator bool() const {
			return static_cast<bool>(code);
		}


		const auto& what() const {
			return msg;
		}
	};


	inline std::ostream& operator<<(std::ostream& os, const Error& e) {
		return (os << e.what());
	}
}

#endif

