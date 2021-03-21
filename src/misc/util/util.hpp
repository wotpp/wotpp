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
#include <misc/colours.hpp>


namespace wpp {
	template <typename T, typename... Ts> constexpr bool eq_any(T&& first, Ts&&... rest) {
		return ((first == rest) or ...);
	}

	template <typename T, typename... Ts> constexpr bool eq_all(T&& first, Ts&&... rest) {
		return ((first == rest) and ...);
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


	#ifndef NDEBUG
		#define DBG(...) \
			do { \
				[fn_name = __func__] (auto&&... args) { \
					auto& os = [&] () -> std::ostream& { \
						return ( \
							std::cerr << ANSI_FG_MAGENTA << "log " << ANSI_RESET << \
							std::filesystem::path(__FILE__).filename().string() << ":" << \
							__LINE__ << " in `" << ANSI_FG_YELLOW << fn_name << ANSI_RESET << "`"); \
					} (); \
					if constexpr(sizeof...(args) > 0) { \
						os << " => " << ANSI_BOLD;\
						(os << ... << std::forward<decltype(args)>(args)) << ANSI_RESET << '\n'; \
					} else { \
						os << ANSI_RESET << '\n'; \
					} \
				} ( __VA_ARGS__ ); \
			} while (false)

	#else
		#define DBG do {} while (false);
	#endif


	struct Error {
		std::string msg;

		void show() const {
			std::cerr << msg;
		}
	};


	template <typename... Ts>
	inline std::string cat(Ts&&... args) {
		const auto tostr = [] (const auto& x) {
			if constexpr(std::is_same_v<std::decay_t<decltype(x)>, std::string>)
				return x;

			else if constexpr(std::is_same_v<std::decay_t<decltype(x)>, char>)
				return std::string(x, 1);

			else if constexpr(std::is_same_v<std::decay_t<decltype(x)>, const char*>)
				return std::string(x);

			else if constexpr(std::is_same_v<std::decay_t<decltype(x)>, wpp::View>)
				return x.str();

			else
				return std::to_string(x);
		};

		return (tostr(args) + ...);
	}


	struct SourceLocation {
		int line = 1, column = 1;
	};


	inline SourceLocation calculate_coordinates(const char* ptr, const char* const end) {
		int line = 1, column = 1;

		for (; ptr < end; ptr += wpp::size_utf8(ptr)) {
			const int cmp = (*ptr == '\n');
			column = (column * not cmp) + 1;
			line += cmp;
		}

		return {line, column};
	}


	inline wpp::Error print_error(
		bool bytes,
		const char* const type,
		const char* const colour,
		const wpp::Pos& pos,
		const wpp::Env& env,
		const std::string& error_general,
		const std::string& error_specific,
		const std::string& suggestion = ""
	) {
		std::ostringstream ss;

		const auto& [source, view] = pos;
		const auto& [offset, length] = view;
		const auto& [file, base, mode] = source;

		// Print path but only if we're not in the REPL.
		std::string pos_info_str;
		std::string src_snippet_str;

		wpp::SourceLocation sloc;

		if (mode != modes::repl) {
			if (bytes) {
				sloc.line = offset - base;
				sloc.column = offset - base;
				pos_info_str = wpp::cat(" ", std::filesystem::relative(file, env.root).string(), ":", offset - base, "(byte)");
			}

			else if (*offset != '\0') {
				sloc = wpp::calculate_coordinates(base, offset);
				pos_info_str = wpp::cat(" ", std::filesystem::relative(file, env.root).string(), ":", sloc.line, ":", sloc.column);
			}

			else {
				pos_info_str = wpp::cat(" ", std::filesystem::relative(file, env.root).string(), ":eof");
			}
		}


		std::string column_str = "    ";

		const char* printout_begin = offset;
		const char* printout_end = offset;

		if (*offset != '\0') {
			// Walk forwards to newline or end of string.
			while (*printout_end != '\n' and *printout_end)
				++printout_end;

			// Walk backwards to newline or beginning of string.
			while (*printout_begin != '\n' and printout_begin > base)
				--printout_begin;

			// Strip whitespace from beginning.
			if (wpp::is_whitespace(printout_begin)) {
				do {
					printout_begin += wpp::size_utf8(printout_begin);
				} while (wpp::is_whitespace(printout_begin));
			}

			// Strip whitespace from end.
			if (wpp::is_whitespace(printout_end)) {
				do {
					printout_end = wpp::prev_char_utf8(base, printout_end);
				} while (wpp::is_whitespace(printout_end));
			}

			if (mode != modes::repl)
				column_str += std::to_string(sloc.line);

			if (not bytes) {
				// Generate source snippet string.
				const std::string printout = std::string(printout_begin, printout_end - printout_begin + 1);
				const std::string specific = wpp::cat(
					std::string(offset - printout_begin, ' '), colour, "╰ ", ANSI_RESET, error_specific, "\n"
				);

				src_snippet_str = wpp::cat(
					column_str, " │ ", printout, "\n",
					std::string(column_str.size(), ' '), " │ ", specific
				);
			}

			else {
				src_snippet_str = wpp::cat(column_str, "(byte) │ ", error_specific);
			}
		}

		else {
			src_snippet_str = wpp::cat(column_str, "(eof) │ ", error_specific);
		}

		if (not suggestion.empty())
			src_snippet_str += wpp::cat("\n    ", ANSI_FG_YELLOW, ANSI_BOLD, "hint: ", ANSI_RESET, ANSI_FG_YELLOW, suggestion, ANSI_RESET, "\n");


		// Print mode if its not modes::normal.
		std::string mode_str;
		if (mode != modes::normal)
			mode_str = wpp::cat("[", modes::mode_to_str[mode], "] ");

		ss << mode_str << colour << type << ANSI_RESET << pos_info_str << " => " << ANSI_BOLD << error_general << ANSI_RESET << '\n' << src_snippet_str << '\n';

		return wpp::Error{ ss.str() };
	}

	inline wpp::Error print_error(
		bool bytes,
		const char* const type,
		const char* const colour,
		const wpp::node_t& node_id,
		const wpp::Env& env,
		const std::string& error_general,
		const std::string& error_specific = "",
		const std::string& suggestion = ""
	) {
		return print_error(bytes, type, colour, env.positions[node_id], env, error_general, error_specific, suggestion);
	}


	// Print an error with position info.
	template <typename... Ts> [[noreturn]] inline void error(Ts&&... args) {
		throw print_error(false, "error", ANSI_FG_RED, std::forward<Ts>(args)...);
	}

	// Print error with position info in byte offset format.
	template <typename... Ts> [[noreturn]] inline void error_utf8(Ts&&... args) {
		throw print_error(true, "error", ANSI_FG_RED, std::forward<Ts>(args)...);
	}

	// Print a warning with position info.
	template <typename... Ts> inline void warn(Ts&&... args) {
		print_error(false, "warning", ANSI_FG_BLUE, std::forward<Ts>(args)...).show();
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
