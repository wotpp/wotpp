#include <string>
#include <iostream>
#include <utility>
#include <chrono>

#include <cstdint>
#include <cstring>
#include <ctime>

#include <structures/warnings.hpp>
#include <visitors/eval.hpp>
#include <repl.hpp>

#include <tinge.hpp>

int main(int argc, const char* argv[]) {
	wpp::warning_t warning_flags =
		wpp::WARN_FUNC_REDEFINED |
		wpp::WARN_PARAM_SHADOW_FUNC |
		wpp::WARN_PARAM_SHADOW_PARAM |
		wpp::WARN_VARFUNC_REDEFINED
	;

	if (argc == 1)
		return wpp::repl();

	else if (argc == 2)
		return wpp::run(argv[1], warning_flags);

	else
		tinge::errorln("usage: wpp [file]");

	return 1;
}
