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
#include <exception.hpp>
#include <visitors/sexpr.hpp>
#include <visitors/eval.hpp>

#include <tinge.hpp>

int main(int argc, const char* argv[]) {
	if (argc != 2) {
		tinge::errorln("usage: wpp <file>");
		return 1;
	}

	auto file = wpp::read_file(argv[1]);

	wpp::Environment env;
	wpp::Backtrace bt;

	try {
		std::cout << wpp::eval(file, env, bt) << std::endl;
	}

	catch (const wpp::BacktraceException& e) {
		std::cout << "Document backtrace:" << std::endl;
		wpp::print_backtrace(e.backtrace);
		wpp::error(e.pos, e.what());
		return 1;
	}

	catch (const wpp::Exception& e) {
		wpp::error(e.pos, e.what());
		return 1;
	}

	return 0;
}
