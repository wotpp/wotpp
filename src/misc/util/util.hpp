#pragma once

#ifndef WOTPP_UTIL
#define WOTPP_UTIL

#include <utility>
#include <iostream>
#include <string>
#include <sstream>
#include <variant>

#include <frontend/position.hpp>

namespace wpp {
	// Concatenate objects with operator<< overload and return string.
	template <typename... Ts>
	inline std::string strcat(Ts&&... args) {
		std::string buf{(sizeof(args) + ...), '\0'};

		std::stringstream ss{buf};
		((ss << std::forward<Ts>(args)), ...);

		return ss.str();
	}


	template <typename T, typename... Ts>
	inline std::string cat(const T& first, Ts&&... args) {
		if constexpr(sizeof...(Ts) > 0) {
			const auto tostr = [] (const auto& x) {
				if constexpr(std::is_same_v<std::decay_t<decltype(x)>, std::string>)
					return x;

				else if constexpr(std::is_same_v<std::decay_t<decltype(x)>, const char*>)
					return x;

				else
					return std::to_string(x);
			};

			return std::string{first} + (tostr(args) + ...);
		}

		else {
			return std::string{first};
		}
	}


	// Get the absolute difference between 2 pointers.
	constexpr ptrdiff_t ptrdiff(const char* a, const char* b) {
		return
			(a > b ? a : b) -
			(a < b ? a : b);
		;
	}


	// Print an error with position info.
	template <typename... Ts>
	inline void error(const wpp::Position& pos, Ts&&... args) {
		([&] () -> std::ostream& {
			return (std::cerr << "error @ " << pos << ": ");
		} () << ... << std::forward<Ts>(args)) << '\n';
	}

	// Print a warning with position info.
	template <typename... Ts>
	inline void warn(const wpp::Position& pos, Ts&&... args) {
		([&] () -> std::ostream& {
			return (std::cerr << "warning @ " << pos << ": ");
		} () << ... << std::forward<Ts>(args)) << '\n';
	}


	// A handy wrapper for visit which accepts alternatives as variadic arguments.
	template <typename... Ts> struct overloaded: Ts... { using Ts::operator()...; };
	template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;

	template <typename V, typename... Ts>
	constexpr decltype(auto) visit(V&& variant, Ts&&... args) {
		return std::visit(wpp::overloaded{
			std::forward<Ts>(args)...
		}, variant);
	}


	// FNV-1a hash.
	// Calculate a hash of a range of bytes.
	uint64_t hash_bytes(const char*, const char* const);


	// Execute a shell command, capture its standard output and return it
	// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
	std::string exec(const std::string&, int&);


	// Pipe string to stdin of a cmd.
	std::string exec(const std::string&, const std::string&, int&);


	// Read a file into a string relatively quickly.
	std::string read_file(const std::string&);
}


#endif
