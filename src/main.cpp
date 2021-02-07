#include <string>
#include <iostream>
#include <utility>
#include <chrono>

#include <cstdint>
#include <cstring>
#include <ctime>

#include <lexer.hpp>
#include <parser.hpp>
#include <genprog.hpp>
#include <visitors/sexpr.hpp>
#include <visitors/eval.hpp>

#include <tinge.hpp>

int main(int argc, const char* argv[]) {
	if (argc != 2) {
		tinge::errorln("usage: wpp <file>");
		return 1;
	}

	auto file = wpp::read_file(argv[1]);
	std::cout << wpp::eval(file) << std::endl;
  
	return 0;
}
