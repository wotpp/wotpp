#pragma once

#ifndef WOTPP_ARGP
#define WOTPP_ARGP

#include <string>
#include <vector>
#include <cstring>

namespace wpp {
	struct ArgResult {
		bool is_present;
		std::string value;
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
		std::vector<Argument> arguments;

		public:
		ArgumentParser(std::string name, std::string desc) {
			app_name = name;
			app_desc = desc;
		}

		ArgumentParser arg(ArgResult* s, std::string d, std::string l, std::string sh, bool v) {
			arguments.push_back({ s, d, l, sh, v });
			return *this;
		}

		void parse(int argc, char** argv) {
			for (int i = 1; i < argc; i++) {
				bool found;
				Argument arg;

				if (std::strcmp(argv[i], "--")) {
					break;
				}

				for (auto a : arguments) {
					if (a.long_name == argv[i] or a.short_name == argv[i]) {
						found = true;
						arg = a;
					}
				}

				if (not found) {
					throw "Invalid argument";
				}

				arg.store->is_present = true;

				if (arg.takes_value) {
					arg.store->value = argv[++i];
				}
			}
		}
	};
}

#endif // WOTPP_ARGP
