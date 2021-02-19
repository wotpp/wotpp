#include <string>
#include <iostream>
#include <utility>
#include <chrono>

#include <cstdint>
#include <cstring>
#include <ctime>

#include <visitors/eval.hpp>
#include <repl.hpp>

#include <tinge.hpp>

int main(int argc, const char* argv[]) {
	if (argc == 1)
		return wpp::repl();

	else if (argc == 2)
		return wpp::run(argv[1]);

	else
		tinge::errorln("usage: wpp [file]");

	return 1;
}
