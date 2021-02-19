#pragma once

#ifndef WOTPP_ARGP
#define WOTPP_ARGP

#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <sstream>

namespace wpp {
	struct ArgResult {
		bool is_present;
		std::string value;

		ArgResult() {
			is_present = false;
			value = "";
		}
	};
	
	struct Argument {
		ArgResult* store;
		std::string description;
		std::string long_name;
		std::string short_name;
		bool takes_value;
	};
	
	class ArgumentParser {
		std::string app_name;
		std::string app_desc;
		std::string app_version;
		std::vector<Argument> arguments;

		public:
		ArgumentParser(std::string name, std::string desc, std::string version) {
			app_name = name;
			app_desc = desc;
			app_version = version;
		}

		ArgumentParser arg(ArgResult* s, std::string d, std::string l, std::string sh, bool v) {
			arguments.push_back({ s, d, l, sh, v });
			return *this;
		}

		bool parse(int argc, const char** argv) {
			if (argc == 1)
				throw "Expected at least 1 argument";
			
			for (int i = 1; i < argc; i++) {
				printf("%d : %s : %ld\n", i, argv[i], strlen(argv[i]));

				if (strcmp(argv[i], "--") == 0) 
					break;

				if (argv[i][0] != '-') {
					std::cout << app_name << ": expected option, found: " << argv[i] << std::endl;
					return false;
				}

				Argument arg;
				bool found = false;

				for (auto a : arguments) {
					bool is_short = a.short_name == argv[i] + 1;
					bool is_long = a.long_name == argv[i] + 2;
					if (is_long or is_short) {
						arg = a;
						found = true;
					}
				}

				if (not found) {
					std::cout << app_name << ": unrecognized option: " << argv[i] << std::endl;
					return false;
				}

				arg.store->is_present = true;

				if (arg.takes_value) {
					if (++i < argc) {
						arg.store->value = std::string(argv[i]);
					} else {
						std::cout << app_name << ": option requires an argument: " << argv[i - 1] << std::endl;
						return false;
					}
				}
			}

			return true;
		}
	};
}

#endif // WOTPP_ARGP
