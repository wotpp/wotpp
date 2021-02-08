#pragma once

#ifndef WOTPP_UTIL
#define WOTPP_UTIL

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <utility>
// #include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <variant>

namespace wpp {
	// Concatenate objects with operator<< overload and return string.
	template <typename... Ts>
	inline std::string strcat(Ts&&... args) {
		std::string buf{(sizeof(args) + ...), '\0'};

		std::stringstream ss{buf};
		((ss << std::forward<Ts>(args)), ...);

		return ss.str();
	}
}

namespace wpp {
	template <typename T, typename... Ts>
	inline std::string cat(const T& first, Ts&&... args) {
		const auto tostr = [] (const auto& x) {
			if constexpr(std::is_same_v<std::decay_t<decltype(x)>, std::string>)
				return x;

			else if constexpr(std::is_same_v<std::decay_t<decltype(x)>, const char*>)
				return x;

			else
				return std::to_string(x);
		};

		if constexpr(sizeof...(Ts) > 0) {
			return std::string{first} + (tostr(args) + ...);
		}

		else {
			return std::string{first};
		}
	}
}

namespace wpp {
	// FNV-1a hash.
	// Calculate a hash of a range of bytes.
	template <typename hash_t = uint64_t>
	constexpr auto hash_bytes(const char* begin, const char* const end) {
		hash_t offset_basis = 0;
		hash_t prime = 0;

		if constexpr(sizeof(hash_t) == sizeof(uint64_t)) {
			offset_basis = 14'695'981'039'346'656'037u;
			prime = 1'099'511'628'211u;
		}

		else if constexpr(sizeof(hash_t) == sizeof(uint32_t)) {
			offset_basis = 2'166'136'261u;
			prime = 16'777'619u;
		}

		hash_t hash = offset_basis;

		while (begin != end) {
			hash = (hash ^ static_cast<hash_t>(*begin)) * prime;
			begin++;
		}

		return hash;
	}
}

// Read a file into a string relatively quickly.
namespace wpp {
	// inline std::string read_file(const std::string& fname) {
	// 	auto filesize = std::filesystem::file_size(fname);
	// 	std::ifstream is(fname, std::ios::binary);

	// 	auto str = std::string(filesize + 1, '\0');
	// 	is.read(str.data(), static_cast<std::streamsize>(filesize));

	// 	return str;
	// }

	inline std::string read_file(const std::string& fname) {
		std::ifstream is(fname, std::ios::in | std::ios::binary);
		std::string str;

		is.seekg(0, std::ios::end);
		str.resize(is.tellg());
		is.seekg(0, std::ios::beg);

		is.read(&str[0], str.size());
		is.close();

		return str;
	}
}

namespace wpp {
	// Get the absolute difference between 2 pointers.
	constexpr ptrdiff_t ptrdiff(const char* a, const char* b) {
		return
			(a > b ? a : b) -
			(a < b ? a : b);
		;
	}
}

namespace wpp {
	// Print an error with position info and exit.
	template <typename T, typename... Ts>
	inline void error(T&& first, Ts&&... args) {
		([&] () -> std::ostream& {
			return (std::cerr << "error @ " << first << " -> ");
		} () << ... << std::forward<Ts>(args)) << '\n';
	}
}

// A handy wrapper for visit which accepts alternatives as variadic arguments.
namespace wpp {
	template <typename... Ts> struct overloaded: Ts... { using Ts::operator()...; };
	template <typename... Ts> overloaded(Ts...) -> overloaded<Ts...>;

	template <typename V, typename... Ts>
	constexpr decltype(auto) visit(V&& variant, Ts&&... args) {
		return std::visit(wpp::overloaded{
			std::forward<Ts>(args)...
		}, variant);
	}
}

namespace wpp {
	// Execute a shell command, capture it's standard output and return it
	// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
	std::string exec(const std::string& cmd) {
		std::array<char, 128> buffer;
		std::string result;
		std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);

		if (!pipe) {
			throw std::runtime_error("popen() failed!");
		}

		while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
			result += buffer.data();
		}

		return result;
	}
}

#endif
