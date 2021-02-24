#pragma once

#ifndef WOTPP_ARGP
#define WOTPP_ARGP

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include <cstring>
#include <cstdio>

namespace wpp {
	struct ArgResult {
		bool is_present = false;
		bool has_value = false;
		std::string value = "";
	};


	struct Argument {
		ArgResult* store;
		std::string description;
		std::string long_name;
		std::string short_name;
		bool takes_value;
	};


	class ArgumentParser {
		private:
			std::string app_name;
			std::string bin_name;
			std::string app_desc;
			std::string app_version;
			std::string app_usage;
			std::vector<Argument> arguments;


		public:
			ArgumentParser(
				const std::string& name,
				const std::string& desc,
				const std::string& version,
				const std::string& usage
			):
				app_name(name),
				app_desc(desc),
				app_version(version),
				app_usage(usage) {}


			ArgumentParser arg(ArgResult* s, const std::string& d, const std::string& l, const std::string& sh, bool v) {
				arguments.push_back({ s, d, l, sh, v });
				return *this;
			}


			void print_help() {
				std::cout << app_name << " v" << app_version << " interpreter\n";
				std::cout << '\n';
				std::cout << "Usage: " << app_usage << '\n';
				std::cout << '\n';
				std::cout << app_desc << '\n';
				std::cout << '\n';

				std::cout << "\t--help (-h)\n\t\tShows this message\n\n";

				for (const auto& [store, description, long_name, short_name, takes_value]: arguments) {
					std::cout << "\t--" << long_name << " (-" << short_name << ") ";

					if (takes_value)
						std::cout << "[VALUE]";

					else
						std::cout << "\t";

					std::cout << "\n\t\t" << description << "\n\n";
				}
			}


			bool parse(int argc, const char** argv) {
				bin_name = argv[0];
				bin_name = bin_name.substr(bin_name.rfind("/") + 1);

				// If no arguments are supplied, print help
				if (argc == 1)
					print_help();

				for (int i = 1; i < argc; i++) {
					// If `--` is encountered, stop parsing
					if (strcmp(argv[i], "--") == 0)
						break;

					// Print help message
					if (strcmp(argv[i], "--help") == 0 or strcmp(argv[i], "-h") == 0) {
						print_help();
						return false;
					}

					// Make sure the argument is a flag (there are no positional arguments)
					if (argv[i][0] != '-') {
						std::cout << bin_name << ": expected option, found: " << argv[i] << '\n';
						print_help();
						return false;
					}

					Argument arg;
					bool found = false;

					// Search for argument provided
					for (auto a : arguments) {
						bool is_short = a.short_name == argv[i] + 1;
						bool is_long = a.long_name == argv[i] + 2;

						if (is_long or is_short) {
							arg = a;
							found = true;
						}
					}

					// Handle unknown options
					if (not found) {
						std::cout << bin_name << ": unrecognized option: " << argv[i] << '\n';
						print_help();
						return false;
					}

					arg.store->is_present = true;

					if (arg.takes_value) {
						if (++i < argc) {
							// Set value
							arg.store->value = std::string(argv[i]);
							arg.store->has_value = true;

						} else {
							// Deal with missing argument
							std::cout << bin_name << ": option requires an argument: " << argv[i - 1] << '\n';
							print_help();
							return false;
						}
					}
				}

				return true;
			}
	};


}

#endif
