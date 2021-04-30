#pragma once

#ifndef WOTPP_LOGGING
#define WOTPP_LOGGING

#include <structures/environment.hpp>
#include <frontend/char.hpp>
#include <frontend/view.hpp>
#include <misc/dbg.hpp>
#include <misc/colours.hpp>

namespace wpp {
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


	// Calculate lines and columns encountered between `ptr` and `end`.
	inline wpp::SourceLocation calculate_coordinates(const char* ptr, const char* const end) {
		DBG();

		int line = 1, column = 1;

		for (; ptr < end; ptr += wpp::size_utf8(ptr)) {
			const int cmp = (*ptr == '\n');
			column = (column * not cmp) + 1;
			line += cmp;
		}

		return {line, column};
	}



	inline std::string generate_position_str(
		wpp::error_mode_type_t error_mode,
		const char* colour,
		const wpp::SourceLocation& sloc,
		const wpp::Pos& pos,
		wpp::Env& env,
		const std::string& overview,
		const std::string& detail,
		const std::string& suggestion
	) {
		DBG();

		const auto& [source, view] = pos;
		const auto& [offset, length] = view;
		const auto& [file, base, mode] = source;
		const auto& [line, column] = sloc;

		std::string str;


		if (mode != modes::repl)
			str = wpp::cat(": ", std::filesystem::relative(file, env.root).string(), ":");


		// UTF-8 error. We print byte offset rather than line & column.
		if (mode != modes::repl and error_mode == error_modes::utf8) {
			str += wpp::cat(offset - base, "(byte)");
		}

		// Normal error/warning. We print line & column or EOF.
		else if (mode != modes::repl and error_mode != error_modes::utf8) {
			if (*offset != '\0')
				str += wpp::cat(line, ":", column);

			else
				str += "eof";
		}


		return str;

	}


	inline std::string generate_snippet_str_full(
		wpp::error_mode_type_t error_mode,
		const char* colour,
		const wpp::SourceLocation& sloc,
		const wpp::Pos& pos,
		wpp::Env& env,
		const std::string& overview,
		const std::string& detail,
		const std::string& suggestion
	) {
		DBG();

		const auto& [source, view] = pos;
		const auto& [offset, length] = view;
		const auto& [file, base, mode] = source;

		std::string str;

		const char* printout_begin = offset;
		const char* printout_end = offset;

		const std::string indent = "  ";


		// Early out for EOF.
		if (*offset == '\0' and mode == modes::repl)
			return wpp::cat(indent, " | ", detail);

		else if (*offset == '\0' and mode != modes::repl)
			return wpp::cat(indent, "(eof) | ", detail);


		// Early out for UTF-8 error.
		if (error_mode == error_modes::utf8)
			return wpp::cat(indent, offset - base, "(byte) | ", detail);


		// Walk forwards to newline or end of string.
		while (*(printout_end + 1) != '\n' and *(printout_end + 1) != '\0')
			++printout_end;

		// Walk backwards to newline or beginning of string.
		while (*(printout_begin - 1) != '\n' and printout_begin > base)
			--printout_begin;


		// Strip whitespace from beginning.
		if (wpp::is_whitespace(printout_begin)) {
			do
				printout_begin += wpp::size_utf8(printout_begin);
			while (wpp::is_whitespace(printout_begin));
		}

		// Strip whitespace from end.
		if (wpp::is_whitespace(printout_end)) {
			do
				printout_end = wpp::prev_char_utf8(base, printout_end);
			while (wpp::is_whitespace(printout_end));
		}

		const auto printout_str = std::string(printout_begin, printout_end - printout_begin + 1);
		const auto column_str = wpp::cat(indent, sloc.line);
		const auto detail_str = wpp::cat(std::string(offset - printout_begin, ' '), colour, "⤷ ", env.lookup_colour(ANSI_RESET), detail);

		str = wpp::cat(
			column_str, " | ", printout_str, "\n",
			std::string(column_str.size(), ' '), " | ",
			detail_str, "\n"
		);

		if (not suggestion.empty())
			str += wpp::cat(
				"\n", indent, env.lookup_colour(ANSI_BOLD), env.lookup_colour(ANSI_FG_YELLOW), "hint: ",
				env.lookup_colour(ANSI_RESET),
				suggestion,
				env.lookup_colour(ANSI_RESET), "\n"
			);

		return str;
	}



	inline std::string generate_snippet_str_inline(
		wpp::error_mode_type_t error_mode,
		const char* colour,
		const wpp::SourceLocation& sloc,
		const wpp::Pos& pos,
		wpp::Env& env,
		const std::string& overview,
		const std::string& detail,
		const std::string& suggestion
	) {
		DBG();

		const auto& [source, view] = pos;
		const auto& [offset, length] = view;
		const auto& [file, base, mode] = source;

		if (not suggestion.empty())
			return wpp::cat(
				"  ", env.lookup_colour(ANSI_BOLD), env.lookup_colour(ANSI_FG_YELLOW), "hint: ",
				env.lookup_colour(ANSI_RESET), suggestion, env.lookup_colour(ANSI_RESET)
			);

		return "";
	}



	struct Report {
		wpp::error_mode_type_t error_mode;

		wpp::Pos pos;
		wpp::Env& env;

		std::string overview;
		std::string detail;
		std::string suggestion = "";


		std::string str() const {
			DBG();

			const auto& [source, view] = pos;
			const auto& [offset, length] = view;
			const auto& [file, base, mode] = source;

			const auto sloc = wpp::calculate_coordinates(base, offset);

			const char* error_str = error_modes::error_mode_to_str[error_mode];
			const char* colour = env.lookup_colour(ANSI_FG_RED);


			if (error_mode == error_modes::error)
				colour = env.lookup_colour(ANSI_FG_RED);

			else if (error_mode == error_modes::utf8)
				colour = env.lookup_colour(ANSI_FG_RED);

			else if (error_mode == error_modes::warning)
				colour = env.lookup_colour(ANSI_FG_BLUE);


			std::string snippet_str;
			const std::string position_str = generate_position_str(error_mode, colour, sloc, pos, env, overview, detail, suggestion);


			if (env.flags & wpp::FLAG_INLINE_REPORTS) {
				snippet_str = generate_snippet_str_inline(error_mode, colour, sloc, pos, env, overview, detail, suggestion);

				return wpp::cat(
					colour, error_str, env.lookup_colour(ANSI_RESET),
					position_str, " => ", env.lookup_colour(ANSI_BOLD),
					detail, env.lookup_colour(ANSI_RESET), "\n",
					snippet_str, "\n"
				);
			}


			snippet_str = generate_snippet_str_full(error_mode, colour, sloc, pos, env, overview, detail, suggestion);

			return wpp::cat(
				colour, error_str, env.lookup_colour(ANSI_RESET),
				position_str, " => ", env.lookup_colour(ANSI_BOLD),
				overview, env.lookup_colour(ANSI_RESET), "\n",
				snippet_str, "\n"
			);
		}
	};



	// Print an error with position info.
	inline wpp::Report generate_error(
		const wpp::Pos& pos,
		wpp::Env& env,
		const std::string& overview,
		const std::string& detail,
		const std::string& suggestion = ""
	) {
		DBG();
		return wpp::Report{ error_modes::error, pos, env, overview, detail, suggestion };
	}

	template <typename... Ts>
	inline wpp::Report generate_error(wpp::node_t node_id, wpp::Env& env, Ts&&... args) {
		return wpp::generate_error(env.positions[node_id], env, std::forward<Ts>(args)...);
	}

	template <typename... Ts>
	[[noreturn]] inline void error(Ts&&... args) {
		throw wpp::generate_error(std::forward<Ts>(args)...);
	}


	// Print error with position info in byte offset format.
	inline wpp::Report generate_error_utf8(
		const wpp::Pos& pos,
		wpp::Env& env,
		const std::string& overview,
		const std::string& detail,
		const std::string& suggestion = ""
	) {
		DBG();
		return wpp::Report{ error_modes::utf8, pos, env, overview, detail, suggestion };
	}

	template <typename... Ts>
	inline wpp::Report generate_error_utf8(wpp::node_t node_id, wpp::Env& env, Ts&&... args) {
		return wpp::generate_error_utf8(env.positions[node_id], env, std::forward<Ts>(args)...);
	}

	template <typename... Ts>
	[[noreturn]] inline void error_utf8(Ts&&... args) {
		throw wpp::generate_error_utf8(std::forward<Ts>(args)...);
	}


	// Warning
	inline wpp::Report generate_warning(
		const wpp::Pos& pos,
		wpp::Env& env,
		const std::string& overview,
		const std::string& detail,
		const std::string& suggestion = ""
	) {
		DBG();
		return wpp::Report{ error_modes::warning, pos, env, overview, detail, suggestion };
	}

	template <typename... Ts>
	inline wpp::Report generate_warning(wpp::node_t node_id, wpp::Env& env, Ts&&... args) {
		return wpp::generate_warning(env.positions[node_id], env, std::forward<Ts>(args)...);
	}

	template <typename... Ts>
	inline void warning(wpp::node_t node_id, wpp::Env& env, Ts&&... args) {
		std::cerr << wpp::generate_warning(node_id, env, std::forward<Ts>(args)...).str();
	}

	template <typename T, typename... Ts>
	inline void warning_once(T warning_type, wpp::node_t node_id, wpp::Env& env, Ts&&... args) {
		// https://stackoverflow.com/questions/56923254/c-map-or-unordered-map-when-key-is-two-integers
		size_t hash = 0;

		hash ^= warning_type + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= node_id + 0x9e3779b9 + (hash << 6) + (hash >> 2);

		if (env.seen_warnings.find(hash) != env.seen_warnings.end())
			return;

		env.seen_warnings.emplace(hash);
		std::cerr << wpp::generate_warning(node_id, env, std::forward<Ts>(args)...).str();
	}
}

#endif
