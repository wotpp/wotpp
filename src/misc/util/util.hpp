#pragma once

#ifndef WOTPP_UTIL
#define WOTPP_UTIL

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <variant>
#include <filesystem>
#include <type_traits>

#include <structures/environment.hpp>
#include <frontend/view.hpp>
#include <frontend/char.hpp>
#include <misc/fwddecl.hpp>
#include <misc/report.hpp>
#include <misc/colours.hpp>
#include <misc/dbg.hpp>


namespace wpp {
	template <typename T, typename... Ts> constexpr bool eq_any(T&& first, Ts&&... rest) {
		return ((first == rest) or ...);
	}

	template <typename T, typename... Ts> constexpr bool eq_all(T&& first, Ts&&... rest) {
		return ((first == rest) and ...);
	}


	inline int view_to_int(const View& v) {
		DBG();

		int n = 0;

		auto ptr = v.ptr;

		if (*ptr == '-')
			++ptr;

		for (; ptr != v.ptr + v.length; ++ptr)
			n = (n * 10) + (*ptr - '0');

		if (*v.ptr == '-')
			n *= -1;

		return n;
	}


	// std::string constructor does not allow repeating a string so
	// this function implements it.
	inline std::string repeat(const std::string& c, std::string::size_type n) {
		if (!n)
			return ""; // Check for 0.

		std::string out = c;
		out.reserve(c.size() * n);

		for (n--; n > 0; n--)
			out += c;

		return out;
	}


	template <typename T1, typename T2> constexpr auto max(T1&& a, T2&& b) {
		return a > b ? a : b;
	}

	template <typename T1, typename T2> constexpr auto min(T1&& a, T2&& b) {
		return a < b ? a : b;
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


	// Execute a shell command, capture its standard output and return it
	// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
	std::string exec(const std::string&, int&);


	// Pipe string to stdin of a cmd.
	std::string exec(const std::string&, const std::string&, int&);


	struct FileError {};


	inline std::filesystem::path get_file_path(const std::filesystem::path& file, const SearchPath& search_path) {
		DBG();

		// Check the current directory.
		if (std::filesystem::exists(std::filesystem::current_path() / file))
			return file;

		// Otherwise, find it in the search path.
		for (const auto& dir: search_path) {
			const auto path = dir / file;

			if (std::filesystem::exists(path))
				return path;
		}

		throw wpp::FileError{};
	}


	// Read a file into a string relatively quickly.
	inline std::string read_file(const std::filesystem::path& path) {
		DBG();

		std::ifstream is(path);

		if (not is.is_open())
			throw wpp::FileError{};

		std::stringstream ss;
		ss << is.rdbuf();

		return ss.str();
	}


	// Write string to file.
	inline void write_file(const std::filesystem::path& path, const std::string& contents) {
		DBG();
		auto file = std::ofstream(path);
		file << contents;
		file.close();
	}
}


#endif
