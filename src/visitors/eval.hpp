#pragma once

#ifndef WOTPP_EVAL
#define WOTPP_EVAL

#include <string>
#include <unordered_map>
#include <stack>
#include <type_traits>

#include <utils/util.hpp>
#include <structures/ast.hpp>
#include <exception.hpp>
#include <parser.hpp>

// AST visitor that evaluates the program.

namespace wpp {
	using Environment = std::unordered_map<std::string, wpp::node_t>;
	using Arguments = std::unordered_map<std::string, std::string>;

	inline std::string eval_ast(const wpp::node_t node_id, wpp::AST& tree, Environment& functions, Backtrace backtrace, Arguments* args = nullptr) {
		const auto& variant = tree[node_id];
		std::string str;

		wpp::visit(variant,
			[&] (const FnRun& run) {
				const auto& [arg, pos] = run;

				auto cmd = eval_ast(arg, tree, functions, backtrace, args);
				str = wpp::exec(cmd);
			},

			[&] (const FnEval& eval) {
				const auto& [arg, pos] = eval;

				auto code = eval_ast(arg, tree, functions, backtrace, args);

				wpp::Lexer lex{code.c_str()};
				wpp::node_t root;
				wpp::Backtrace bt; // create an individual backtrace for every eval

				try {
					root = document(lex, tree);
					str = wpp::eval_ast(root, tree, functions, bt, args);
				}

				catch (const wpp::BacktraceException& e) {
					// We caught a backtrace exception, print the backtrace's exception and throw our own backtrace for the upper-level eval to handle
					if(!e.backtrace.empty()) {
						std::cerr << "Eval backtrace @ " << eval.pos << ":" << std::endl;
						wpp::print_backtrace(e.backtrace);
					}

					throw wpp::BacktraceException{backtrace, pos, "inside eval: ", e.what()};
				}

				catch (const wpp::Exception& e) {
					// We caught a generic exception, throw that with backtrace for the upper-level eval to handle (we are the lowermost level)
					throw wpp::BacktraceException{backtrace, pos, "inside eval: ", e.what()};
				}
			},

			[&] (const FnAssert& ass) {
				const auto& [arg, pos] = ass;
			},

			[&] (const FnFile& file) {
				const auto& [arg, pos] = file;
			},

			[&] (const FnInvoke& call) {
				const auto& [caller_name, caller_args, caller_pos] = call;
				std::string caller_mangled_name = cat(caller_name, caller_args.size());

				// Check if parameter.
				if (args) {
					if (auto it = (*args).find(caller_name); it != (*args).end()) {
						str = it->second;
						return;
					}
				}

				// Add it to the backtrace
				backtrace.push_back(call);

				// If it wasn't a parameter, we fall through to here and check if it's a function.
				auto it = functions.find(caller_mangled_name);
				if (it == functions.end())
					throw wpp::BacktraceException{backtrace, caller_pos, "func not found: ", caller_name};

				// Retrieve function.
				const auto& [callee_name, params, body, callee_pos] = tree.get<wpp::Fn>(it->second);

				// Set up Arguments to pass down to function body.
				Arguments env_args;

				// Evaluate arguments and store their result.
				for (int i = 0; i < (int)caller_args.size(); i++)
					env_args.emplace(params[i], eval_ast(caller_args[i], tree, functions, backtrace, args));

				// Call function.
				str = eval_ast(body, tree, functions, backtrace, &env_args);
			},

			[&] (const Fn& func) {
				const auto& [name, params, body, pos] = func;
				functions.insert_or_assign(cat(name, params.size()), node_id);
			},

			[&] (const String& x) {
				str = x.value;
			},

			[&] (const Concat& cat) {
				const auto& [lhs, rhs, pos] = cat;
				str = eval_ast(lhs, tree, functions, backtrace, args) + eval_ast(rhs, tree, functions, backtrace, args);
			},

			[&] (const Block& block) {
				const auto& [stmts, expr, pos] = block;

				for (const wpp::node_t node: stmts) {
					str += eval_ast(node, tree, functions, backtrace, args);
				}

				str = eval_ast(expr, tree, functions, backtrace, args);
			},

			[&] (const Ns&) {
				// const auto& [name, stmts] = ns;
			},

			[&] (const Document& doc) {
				for (const wpp::node_t node: doc.stmts) {
					str += eval_ast(node, tree, functions, backtrace, args);
				}
			}
		);

		return str;
	}

	inline std::string eval(const std::string& code, wpp::Environment& env, wpp::Backtrace backtrace) {
		// Create a new lexer and syntax tree
		wpp::Lexer lex{code.c_str()};
		wpp::AST tree;

		// Reserve 10MiB
		tree.reserve((1024 * 1024 * 10) / sizeof(decltype(tree)::value_type));

		// Parse.
		auto root = document(lex, tree);

		// Evaluate.
		return wpp::eval_ast(root, tree, env, backtrace);
	}

}

#endif

