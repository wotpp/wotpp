#pragma once

#ifndef WOTPP_UTIL
#define WOTPP_UTIL

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <variant>
#include <filesystem>

#include <structures/environment.hpp>
#include <misc/fwddecl.hpp>


namespace wpp {
	#ifndef NDEBUG
		template <typename... Ts>
		inline std::ostream& dbg(Ts&&... args) {
			return ([&] () -> std::ostream& {
				return (std::cerr << "[DEBUG] ");
			} () << ... << std::forward<Ts>(args)) << '\n';
		}

	#else
		template <typename... Ts>
		inline std::ostream& dbg(Ts&&... args) {
			((void)args, ...); // hide unused warnings.
			return std::cerr;
		}
	#endif


	struct Error {};


	template <typename T, typename... Ts>
	inline std::string cat(const T& first, Ts&&... args) {
		if constexpr(sizeof...(Ts) > 0) {
			const auto tostr = [] (const auto& x) {
				if constexpr(std::is_same_v<std::decay_t<decltype(x)>, std::string>)
					return x;

				else if constexpr(std::is_same_v<std::decay_t<decltype(x)>, const char*>)
					return std::string{x};

				else
					return std::to_string(x);
			};

			return std::string{first} + (tostr(args) + ...);
		}

		else {
			return std::string{first};
		}
	}


	struct SourceLocation {
		int line = 1, column = 1;
	};


	inline SourceLocation calculate_coordinates(const char* ptr, const char* const end) {
		int line = 1, column = 1;

		for (; ptr != end; ++ptr) {
			const int cmp = (*ptr == '\n');
			column = (column * not cmp) + 1;
			line += cmp;
		}

		return {line, column};
	}


	template <typename... Ts>
	inline void print_error(const char* const type, const wpp::Pos& pos, const wpp::Env& env, Ts&&... args) {
		([&] () -> std::ostream& {
			const auto& [ast, functions, positions, root, warning_flags, sources] = env;
			const auto& [source, offset] = pos;
			const auto& [file, base, mode] = source;

			const auto [line, column] = wpp::calculate_coordinates(base, offset);

			// Print path but only if it's not the same as the root path.
			std::string path_str;
			if (file != root)
				path_str = std::filesystem::relative(file, root).string() + ":";

			// Print mode if its not modes::normal.
			std::string mode_str;
			if (mode != modes::normal)
				mode_str = wpp::cat("(", modes::mode_to_str[mode], ") ");

			return (std::cerr << mode_str << type << " @ [" << path_str << line << ":" << column << "]: ");
		} () << ... << std::forward<Ts>(args)) << '\n';
	}


	// Print an error with position info.
	template <typename... Ts>
	inline void error(const wpp::Pos& pos, const wpp::Env& env, Ts&&... args) {
		print_error("error", pos, env, std::forward<Ts>(args)...);
		throw wpp::Error{};
	}


	// Print a warning with position info.
	template <typename... Ts>
	inline void warn(const wpp::Pos& pos, wpp::Env& env, Ts&&... args) {
		print_error("warning", pos, env, std::forward<Ts>(args)...);
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


	// Read a file into a string relatively quickly.
	inline std::string read_file(const std::filesystem::path& path) {
		std::ifstream is(path);

		std::stringstream ss;
		ss << is.rdbuf();

		return ss.str();
	}


	// Write string to file.
	inline void write_file(const std::filesystem::path& path, const std::string& contents) {
		auto file = std::ofstream(path);
		file << contents;
		file.close();
	}
}


#endif
