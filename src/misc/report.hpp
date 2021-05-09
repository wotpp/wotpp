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
		wpp::report_type_type_t report_type,
		wpp::report_mode_type_t report_mode,
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
		if (mode != modes::repl and report_mode == report_modes::encoding) {
			str += wpp::cat(offset - base, "(byte)");
		}

		// Normal error/warning. We print line & column or EOF.
		else if (mode != modes::repl and report_mode != report_modes::encoding) {
			if (*offset != '\0')
				str += wpp::cat(line, ":", column);

			else
				str += "eof";
		}


		return str;

	}


	inline std::string generate_snippet_str(
		wpp::report_type_type_t report_type,
		wpp::report_mode_type_t report_mode,
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
			return wpp::cat(indent, " | ", detail, "\n");

		else if (*offset == '\0' and mode != modes::repl)
			return wpp::cat(indent, "(eof) | ", detail, "\n");


		// Early out for UTF-8 error.
		if (report_mode == report_modes::encoding)
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



	struct Report {
		wpp::report_type_type_t report_type;
		wpp::report_mode_type_t report_mode;

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

			const char* report_type_str = report_types::report_type_to_str[report_type];
			const char* report_mode_str = report_modes::report_mode_to_str[report_mode];
			const char* mode_str = modes::mode_to_str[mode];

			const char* colour = env.lookup_colour(ANSI_FG_RED);


			if (report_type == report_types::error)
				colour = env.lookup_colour(ANSI_FG_RED);

			else if (report_type == report_types::warning)
				colour = env.lookup_colour(ANSI_FG_BLUE);


			const std::string report_str = wpp::cat("(", mode_str, ") ", colour, report_mode_str, " ", report_type_str, env.lookup_colour(ANSI_RESET));


			std::string snippet_str;
			const std::string position_str = generate_position_str(report_type, report_mode, colour, sloc, pos, env, overview, detail, suggestion);


			if (env.flags & wpp::FLAG_INLINE_REPORTS) {
				return wpp::cat(
					report_str,
					position_str, " => ",
					detail, "\n"
				);
			}


			snippet_str = generate_snippet_str(report_type, report_mode, colour, sloc, pos, env, overview, detail, suggestion);

			return wpp::cat(
				report_str,
				position_str, " => ", env.lookup_colour(ANSI_BOLD),
				overview, env.lookup_colour(ANSI_RESET), "\n",
				snippet_str, "\n"
			);
		}
	};



	// Print an error with position info.
	inline wpp::Report generate_error(
		wpp::report_mode_type_t report_mode,
		const wpp::Pos& pos,
		wpp::Env& env,
		const std::string& overview,
		const std::string& detail,
		const std::string& suggestion = ""
	) {
		DBG();
		return wpp::Report{ report_types::error, report_mode, pos, env, overview, detail, suggestion };
	}

	template <typename... Ts>
	inline wpp::Report generate_error(wpp::report_mode_type_t report_mode, wpp::node_t node_id, wpp::Env& env, Ts&&... args) {
		return wpp::generate_error(report_mode, env.ast_meta[node_id].position, env, std::forward<Ts>(args)...);
	}

	template <typename... Ts>
	[[noreturn]] inline void error(Ts&&... args) {
		throw wpp::generate_error(std::forward<Ts>(args)...);
	}


	// Warning
	inline wpp::Report generate_warning(
		wpp::report_mode_type_t report_mode,
		const wpp::Pos& pos,
		wpp::Env& env,
		const std::string& overview,
		const std::string& detail,
		const std::string& suggestion = ""
	) {
		DBG();
		return wpp::Report{ report_types::warning, report_mode, pos, env, overview, detail, suggestion };
	}

	template <typename... Ts>
	inline wpp::Report generate_warning(wpp::report_mode_type_t report_mode, wpp::node_t node_id, wpp::Env& env, Ts&&... args) {
		return wpp::generate_warning(report_mode, env.ast_meta[node_id].position, env, std::forward<Ts>(args)...);
	}

	template <typename... Ts>
	inline void warning(Ts&&... args) {
		std::cerr << wpp::generate_warning(std::forward<Ts>(args)...).str();
	}


	template <typename T1, typename T2>
	constexpr size_t combine(T1&& lhs, T2&& rhs) {
		// https://stackoverflow.com/questions/56923254/c-map-or-unordered-map-when-key-is-two-integers
		size_t hash = 0;

		hash ^= lhs + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		hash ^= rhs + 0x9e3779b9 + (hash << 6) + (hash >> 2);

		return hash;
	}


	template <typename T>
	inline bool is_previously_seen_warning(T warning_type, wpp::node_t node, wpp::Env& env) {
		if (env.seen_warnings.find(wpp::combine(warning_type, node)) != env.seen_warnings.end())
			return true;

		const node_t first = node;

		while (
			env.seen_warnings.find(wpp::combine(warning_type, node)) == env.seen_warnings.end() and
			node != wpp::NODE_ROOT
		)
			node = env.ast_meta[node].parent;

		env.seen_warnings.emplace(wpp::combine(warning_type, first));

		return false;
	}
}

#endif
