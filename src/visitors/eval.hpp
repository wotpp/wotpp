#pragma once

#ifndef WOTPP_EVAL
#define WOTPP_EVAL

#include <string>
#include <unordered_map>
#include <stack>

#include <utils/util.hpp>
#include <structures/ast.hpp>
#include <parser.hpp>

// AST visitor that evaluates the program.

namespace wpp {
	using Environment = std::unordered_map<std::string, wpp::Fn>;
	using Arguments = std::unordered_map<std::string, std::string>;


	template <typename... Ts>
	inline std::string mangle(const std::string& first, Ts&&... args) {
		using namespace std;
		const auto helper = [] (auto&& x) {
			return std::to_string(x) + "__";
		};

		return first + "__" + (helper(args) + ...);
	}

	inline std::string eval(const std::string& code);

	template <typename T>
	inline std::string eval_ast(const T& variant, const wpp::AST& tree, Environment& functions, Arguments* args = nullptr) {
		std::string str;

		wpp::visit(variant,
				   [&] (const FnInvoke& call) {
					   const auto& [caller_name, caller_args, caller_pos] = call;
					   std::string caller_mangled_name = mangle(caller_name, caller_args.size());

					   // Parameter.
					   if (args != nullptr) {
						   auto man_it = (*args).find(caller_name);
						   if (man_it != (*args).end()) {
							   str = man_it->second;
							   return;
						   }

						   // if its not an arg, maybe its a function?
					   }

					   // Function.
					   // If it is a call to shell built-in
					   if(caller_name == "run") {
						   #ifndef WPP_DISABLE_RUN
						   // Set up arguments in environment.
						   std::string command = "";

						   for (int i = 0; i < (int)caller_args.size(); i++) {
							   auto retstr = eval_ast(tree[caller_args[i]], tree, functions, args);
							   command += retstr + " ";
						   }

						   // Run the command
						   str = wpp::exec(command);
						   #else
						   wpp::error(caller_pos, "executing shell commands has been disabled during compilation");
						   std::exit(1);
                           #endif //WPP_DISABLE_RUN
					   }

					   // If it is a call to the eval built-in
					   else if(caller_name == "eval") {
						   if(caller_args.size() != 1) {
							   wpp::error(caller_pos, "call to eval expected 1 argument, got ", caller_args.size(), " instead");
							   std::exit(1);
						   }

						   // Eval the first argument to get the string
						   std::string evalstr = eval_ast(tree[caller_args[0]], tree, functions, args);
						   str = wpp::eval(evalstr);
					   }

					   // If it is just a regular function
					   else {
						   auto it = functions.find(caller_mangled_name);
						   if (it == functions.end()) {
							   // tinge::errorln("func not found: ", caller_mangled_name);
							   wpp::error(caller_pos, "func not found: ", caller_mangled_name);
							   std::exit(1);
						   }

						   // Use function that was looked up.
						   const auto& [callee_name, params, body, callee_pos] = it->second;

						   // Set up arguments in environment.
						   Arguments env_args;

						   for (int i = 0; i < (int)caller_args.size(); i++) {
							   auto retstr = eval_ast(tree[caller_args[i]], tree, functions, args);
							   env_args.emplace(params[i], retstr);
						   }

						   // Call function.
						   str = eval_ast(tree[body], tree, functions, &env_args);
					   }
				   },

				   [&] (const Fn& func) {
					   const auto& [name, params, body, pos] = func;
					   functions.insert_or_assign(mangle(name, params.size()), func);
				   },

				   [&] (const String& x) {
					   str = x.value;
				   },

				   [&] (const Concat& cat) {
					   const auto& [lhs, rhs, pos] = cat;
					   str = eval_ast(tree[lhs], tree, functions, args) + eval_ast(tree[rhs], tree, functions, args);
				   },

				   [&] (const Block& block) {
					   const auto& [stmts, expr, pos] = block;

					   for (const wpp::node_t node: stmts) {
						   str += eval_ast(tree[node], tree, functions, args);
					   }

					   str = eval_ast(tree[expr], tree, functions, args);
				   },

				   [&] (const Ns&) {
					   // const auto& [name, stmts] = ns;
				   },

				   [&] (const Document& doc) {
					   for (const wpp::node_t node: doc.exprs_or_stmts) {
						   str += eval_ast(tree[node], tree, functions, args);
					   }
				   }
			);

		return str;
	}

	inline std::string eval(const std::string& code) {
		// Create a new lexer and syntax tree
		wpp::Lexer lex{code.c_str()};
		wpp::AST tree;

		// Reserve 10MiB
		tree.reserve((1024 * 1024 * 10) / sizeof(decltype(tree)::value_type));

		// Parse.
		auto root = document(lex, tree);

		// Evalutate.
		wpp::Environment env;
		return wpp::eval_ast(tree[root], tree, env);
	}

}

#endif

