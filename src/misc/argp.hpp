#pragma once

#ifndef WOTPP_ARGP
#define WOTPP_ARGP

#include <string>
#include <array>
#include <vector>
#include <iostream>
#include <utility>
#include <string_view>
#include <type_traits>

#include <cstring>

namespace wpp {
	using err_t = int;
	enum: err_t {
		SUCCESS         = 0b0000'0000,
		ERR_UNKNOWN_ARG = 0b0000'0001,
		ERR_TAKES_ARG   = 0b0000'0010,
		ERR_NO_ARG      = 0b0000'0100,
		ERR_NO_INLINE   = 0b0000'1000,
		ERR_NO_EQUAL    = 0b0001'0000,
		SHOW_HELP       = 0b0010'0000,
	};


	// Represents a commandline argument.
	template <typename T>
	struct Opt {
		T& ref;
		const char* const desc;
		const char* const long_opt;
		const char* const short_opt;
	};


	struct Meta {
		const char* const version;
		const char* const desc;
	};


	template <typename T, typename... Ts>
	Opt(T, Ts...) -> Opt<T>;


	// Attempt to parse a string with a given opt parser.
	template <typename T>
	inline bool option_parse(int& i, int argc, const char* argv[], err_t& err, const Opt<T>& parser) {
		const auto& [ref, desc, lng, shrt] = parser;

		using RefT = std::decay_t<decltype(ref)>;


		// Check if we recognise this argument.
		const char* str = argv[i];

		auto len = std::strlen(argv[i]);
		auto len_lng = std::strlen(lng);
		auto len_shrt = std::strlen(shrt);

		bool is_short = std::strncmp(shrt, argv[i], len_shrt) == 0;
		bool is_long = std::strncmp(lng, argv[i], len_lng) == 0;


		bool has_inline_arg =
			(is_short and len > len_shrt) or
			(is_long and len > len_lng)
		;


		// Increment the str pointer to past the argument name so we can access the value.
		if (is_short)
			str = argv[i] + len_shrt;

		else if (is_long)
			str = argv[i] + len_lng;


		bool has_equal_arg = str[0] == '=';


		// Check if we recognise this option.
		if (std::strncmp("--help", argv[i], len) == 0 or std::strncmp("-h", argv[i], len) == 0) {
			// This will not be handled as an error but because it is
			// also not success, the help message will be printed.
			err = SHOW_HELP;
			return false;
		}

		else if (not is_long and not is_short) {
			err |= ERR_UNKNOWN_ARG;
			return false;
		}


		// Skip `=` is we are an equal arg.
		if (has_equal_arg)
			++str;


		// Check if arg is a flag.
		if constexpr(std::is_same_v<RefT, bool>) {
			// Flags don't support inline or equal arguments.
			if (has_inline_arg or has_equal_arg) {
				err |= ERR_NO_ARG;
				return false;
			}

			ref = not ref;
		}


		// Arg must take a value.
		else {
			// Long options don't support inline args _unless_ there's an equal sign.
			if (has_inline_arg and is_long and not has_equal_arg) {
				err |= ERR_NO_INLINE;
				return false;
			}

			// Short options don't support equal args.
			else if (has_equal_arg and is_short and has_equal_arg) {
				err |= ERR_NO_EQUAL;
				return false;
			}

			// If we don't have inline args we must have an argument at the next index.
			else if (not has_inline_arg) {
				// Check if accessing the next element would result in out of bounds.
				if (i + 1 > argc - 1) {
					err |= ERR_TAKES_ARG;
					return false;
				}

				// Check if argument is `--`.
				else if (std::strcmp(argv[i + 1], "--") == 0) {
					err |= ERR_TAKES_ARG;
					return false;
				}

				str = argv[++i];
			}


			// Handle all the types of values.
			if constexpr(std::is_same_v<RefT, std::string_view>)
				ref = str;

			else if constexpr(std::is_same_v<RefT, std::vector<std::string_view>>) {
				// Extract each option from the comma seperated list.
				const char* begin = str;
				const char* end = begin;

				// Multiple occurences of a flag are allowed but the most recent one overrides old values.
				ref.clear();
				ref.reserve(ref.capacity() + 10);

				// Loop until end of string.
				while (*end) {
					// If we find a comma, push back a string_view of the
					// range [begin, end]
					if (*end == ',') {
						if (end != begin)
							ref.emplace_back(begin, end - begin);

						// Skip over comma.
						begin = end + 1;
					}

					end++;
				}

				// Emplace the trailing argument.
				if (end != begin)
					ref.emplace_back(begin, end - begin);
			}

			static_assert(
				std::is_same_v<RefT, std::string_view> or
				std::is_same_v<RefT, std::vector<std::string_view>>,
				"reference type is not handled by the argument parser."
			);
		}

		// We succeeded in parsing this argument.
		err = SUCCESS;
		return true;
	}


	// Concatenate a variadic pack of strings to an out parameter.
	template <typename... Ts>
	inline void cat(std::string& out, Ts&&... strings) {
		out.reserve(out.capacity() + sizeof...(Ts) * (sizeof(void*) * 2));
		((out += strings), ...);
	}


	// Generate doc string from an option.
	template <typename T>
	inline std::string option_doc_first_column(const Opt<T>& opt) {
		const auto& [ref, desc, lng, shrt] = opt;

		using RefT = std::remove_reference_t<std::remove_cv_t<decltype(ref)>>;

		std::string doc;

		cat(doc, "  ", shrt, ", ", lng);

		if constexpr(std::is_same_v<RefT, std::string_view>)
			doc += " <value>";

		else if constexpr(std::is_same_v<RefT, std::vector<std::string_view>>)
			doc += " <values>...";

		return doc;
	}


	template <typename T>
	inline void option_doc_second_column(std::string& str, const Opt<T>& opt, int padding) {
		const auto& [ref, desc, lng, shrt] = opt;
		str.reserve(str.capacity() + padding + std::strlen(desc) + 1);

		// using RefT = std::remove_reference_t<std::remove_cv_t<decltype(ref)>>;

		while (padding--)
			str += ' ';

		// if constexpr(std::is_same_v<RefT, bool>)
		// 	cat(str, desc, " (default: ", std::array{"false", "true"}[ref], ")\n");

		// else
			cat(str, desc, '\n');
	}


	template <typename... Ts>
	inline std::string generate_help(const char* const name, const Meta& meta, int padding, Ts&&... opts) {
		const auto& [ver, desc] = meta;
		std::string str;

		// Help header.
		cat(str, name, " ", ver, ": ", desc, "\n\n");

		// Generate the first column for all opts.
		const std::array<std::string, sizeof...(Ts)> help_strs {{
			option_doc_first_column(std::forward<Ts>(opts))...
		}};

		// Find longest opt.
		std::string::size_type max = 0;
		for (const std::string& x: help_strs)
			max = x.size() > max ? x.size(): max;

		// Generate second column taking into account the padding needed to line them up.
		int i = 0;

		([&] (const auto& opt) {
			cat(str, help_strs[i]);
			option_doc_second_column(str, opt, padding + max - help_strs[i].size());
			i++;
		} (std::forward<Ts>(opts)), ...);

		return str;
	}


	template <typename... Ts>
	inline std::string generate_usage(const char* const name, bool has_positional, Ts&&... opts) {
		std::string str, long_opts, short_opts = " [ ";

		cat(str, "usage: ", name);

		([&] (const auto& opt) {
			const auto& [ref, desc, lng, shrt] = opt;

			using RefT = std::remove_reference_t<std::remove_cv_t<decltype(ref)>>;

			if constexpr(std::is_same_v<RefT, bool>)
				cat(short_opts, shrt, " ");

			else {
				if constexpr(std::is_same_v<RefT, std::string_view>)
					cat(long_opts, " [ ", shrt, " <value> ]");

				else if constexpr(std::is_same_v<RefT, std::vector<std::string_view>>)
					cat(long_opts, " [ ", shrt, " <values>... ]");
			}

		} (std::forward<Ts>(opts)), ...);

		cat(str, short_opts, "]", long_opts);

		if (has_positional)
			cat(str, " [ values... ]");

		str += '\n';

		return str;
	}


	const char* binary_name(const char* argv0) {
		for (const char* ptr = argv0 + std::strlen(argv0); ptr != argv0; --ptr) {
			if (*ptr == '/') {
				argv0 = ptr + 1;
				break;
			}
		}

		return argv0;
	}


	// Takes a vector of arguments and a variadic pack
	// of opt parsers and then attempts to parse all of the
	// arguments.
	template <typename... Ts>
	inline bool argparser(
		const Meta& meta,
		const int argc, const char* argv[],
		std::vector<const char*>* positional,
		Ts&&... opts
	) {
		enum {
			SHOULD_EXIT = true,
			SHOULD_CONTINUE = false,
		};


		// Get the binary name by reading backwards to the first `/`.
		const auto bin = binary_name(argv[0]);


		// Implicit help flag.
		bool want_help = false;
		const auto help_opt = Opt{want_help, "print help", "--help", "-h"};


		// Helper for usage string.
		const auto usage = [&] {
			std::cout << generate_usage(bin, positional, help_opt, std::forward<Ts>(opts)...);
		};

		// Helper for, well, help.
		const auto help = [&] {
			usage();
			std::cout << '\n' << generate_help(bin, meta, 2, help_opt, std::forward<Ts>(opts)...);
		};


		// No arguments passed.
		if (argc == 1) {
			usage();
			return SHOULD_EXIT;
		}

		if (positional)
			positional->reserve(positional->capacity() + sizeof...(opts));


		// Loop through argv and try parsing every index
		// with all of the opt parsers.
		for (int i = 1; i != argc; ++i) {
			const auto arg_name = argv[i];
			const auto len = std::strlen(arg_name);

			// Check for end of arguments marker `==`.
			if (std::strncmp("--", arg_name, len) == 0)
				return SHOULD_CONTINUE;

			else {
				// Try parsing the argument, if it fails, we either found
				// an unknown option or have reached the positional arguments.
				err_t flag = SUCCESS;

				if (
					not (
						option_parse(i, argc, argv, flag, std::forward<Ts>(opts)) or ... or
						option_parse(i, argc, argv, flag, help_opt)
					)
				) {
					if (flag & ERR_TAKES_ARG)
						std::cerr << "option '" << arg_name << "' takes an argument." << '\n';

					else if (flag & ERR_NO_ARG)
						std::cerr << "argument passed to flag: '" << arg_name << "'.\n";

					else if (flag & ERR_NO_INLINE)
						std::cerr << "long options do not support inline arguments: '" << arg_name << "'.\n";

					else if (flag & ERR_NO_EQUAL)
						std::cerr << "short options do not support equal arguments: '" << arg_name << "'.\n";

					// Must check this last because while the ERR_UNKNOWN_ARG flag might be set,
					// it could just be because one of the parsers failed to pick it up.
					else if (flag & ERR_UNKNOWN_ARG and argv[i][0] == '-')
						std::cerr << "unknown option '" << arg_name << "'.\n";

					// If no errors occured and SHOW_HELP is not set, this must
					// be a positional argument.
					else if (flag != SHOW_HELP) {
						if (positional)
							positional->emplace_back(argv[i]);

						continue; // Continue here so we don't hit the help message below.
					}


					// If any errors occured, print help and exit.
					if (flag != SUCCESS) {
						help();
						return SHOULD_EXIT;
					}
				}
			}
		}

		return SHOULD_CONTINUE;
	}
}

#endif
