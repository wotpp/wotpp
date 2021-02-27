#include <string>
#include <iostream>
#include <utility>
#include <chrono>

#include <cstdint>
#include <cstring>
#include <ctime>

#include <misc/warnings.hpp>
#include <backend/eval/eval.hpp>
#include <misc/repl.hpp>
#include <misc/argp.hpp>


constexpr auto ver = "alpha-git";
constexpr auto desc = "A small macro language for producing and manipulating strings.";


int main(int argc, const char* argv[]) {
	std::string_view outputf;
	std::vector<std::string_view> warnings;
	bool repl = false;


	std::vector<const char*> positional;

	if (wpp::argparser(
		wpp::Meta{ver, desc},
		argc, argv, &positional,
		wpp::Opt{outputf,  "output file",       "--output",   "-o"},
		wpp::Opt{repl,     "repl mode",         "--repl",     "-R"},
		wpp::Opt{warnings, "toggle warnings",   "--warnings", "-W"}
	))
		return 0;


	wpp::warning_t warning_flags = 0;

	for (const auto& x: warnings) {
		if (x == "param-shadow-func")
			warning_flags |= wpp::WARN_PARAM_SHADOW_FUNC;

		else if (x == "param-shadow-param")
			warning_flags |= wpp::WARN_PARAM_SHADOW_PARAM;

		else if (x == "func-redefined")
			warning_flags |= wpp::WARN_FUNC_REDEFINED;

		else if (x == "varfunc-redefined")
			warning_flags |= wpp::WARN_VARFUNC_REDEFINED;

		else if (x == "all")
			warning_flags = wpp::WARN_ALL;

		else {
			std::cerr << "unrecognized warning: '" << x << "'.\n";
			return 1;
		}
	}


	if (repl)
		return wpp::repl();


	if (positional.empty()) {
		std::cerr << "no input files.\n";
		return 1;
	}


	std::string out;
	const auto initial_path = std::filesystem::current_path();

	for (const auto& fname: positional) {
		try {
			std::string file = wpp::read_file(fname);

			// Set current path to path of file.
			const auto path = std::filesystem::current_path() / std::filesystem::path{fname};
			std::filesystem::current_path(path.parent_path());

			wpp::Lexer lex{std::filesystem::relative(path, initial_path), file.c_str()};
			wpp::AST tree;
			wpp::Environment env{initial_path, tree, warning_flags};

			tree.reserve((1024 * 1024 * 10) / sizeof(decltype(tree)::value_type));

			auto root = wpp::document(lex, tree);
			out += wpp::eval_ast(root, env) + "\n";
		}

		catch (const wpp::Exception& e) {
			wpp::error(e.pos, e.what());
			return 1;
		}

		catch (const std::filesystem::filesystem_error& e) {
			std::cerr << "file '" << fname << "' not found.\n";
			return 1;
		}

		std::filesystem::current_path(initial_path);
	}

	if (not outputf.empty())
		wpp::write_file(outputf, out);

	else
		std::cout << out;


	return 0;
}
