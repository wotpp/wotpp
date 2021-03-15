#include <string>
#include <vector>
#include <algorithm>

#include <misc/util/util.hpp>
#include <misc/flags.hpp>
#include <structures/environment.hpp>
#include <frontend/lexer/lexer.hpp>
#include <frontend/parser/ast_nodes.hpp>
#include <backend/eval/intrinsics.hpp>


namespace wpp {
	// The core of the evaluator.
	std::string evaluate(const wpp::node_t node_id, wpp::Env& env, wpp::FnEnv* fn_env) {
		const auto& variant = env.ast[node_id];
		std::string str;

		wpp::visit(variant,
			[&] (const Intrinsic& fn) {
				wpp::dbg("(eval) intrinsic");

				auto& [ast, functions, positions, root, warning_flags, sources] = env;
				const auto& [type, name, exprs] = fn;

				#define INTRINSIC(n, fn, tok) \
					if (type == tok) { \
						if (n != exprs.size()) \
							wpp::error(positions[node_id], env, name, " takes exactly ", n, " arguments."); \
						str = wpp::intrinsic_##fn(node_id, exprs, env, fn_env); \
						return; \
					}

				INTRINSIC(3, slice,  TOKEN_SLICE);
				INTRINSIC(2, find,   TOKEN_FIND);
				INTRINSIC(2, assert, TOKEN_ASSERT);
				INTRINSIC(2, pipe,   TOKEN_PIPE);
				INTRINSIC(1, error,  TOKEN_ERROR);
				INTRINSIC(1, file,   TOKEN_FILE);
				INTRINSIC(1, escape, TOKEN_ESCAPE);
				INTRINSIC(1, eval,   TOKEN_EVAL);
				INTRINSIC(1, run,    TOKEN_RUN);
				INTRINSIC(1, source, TOKEN_SOURCE);
				INTRINSIC(1, length, TOKEN_LENGTH);
				INTRINSIC(1, log,    TOKEN_LOG);

				#undef INTRINSIC
			},

			[&] (const FnInvoke& call) {
				wpp::dbg("(eval) call");

				auto& [ast, functions, positions, root, warning_flags, sources] = env;

				const auto& [caller_name, caller_args] = call;
				std::string caller_mangled_name = wpp::cat(caller_name, caller_args.size());

				// Check if parameter.
				if (fn_env) {
					auto& args = fn_env->args;

					if (auto it = args.find(caller_name); it != args.end()) {
						if (caller_args.size() > 0)
							wpp::error(positions[node_id], env, "calling argument '", caller_name, "' as if it were a function.");

						str = it->second;

						// Check if it's shadowing a function (even this one).
						if (warning_flags & wpp::WARN_PARAM_SHADOW_FUNC and functions.find(caller_mangled_name) != functions.end())
							wpp::warn(positions[node_id], env, "parameter ", caller_name, " is shadowing a function.");

						return;
					}
				}

				// If it wasn't a parameter, we fall through to here and check if it's a function.
				auto it = functions.find(caller_mangled_name);

				if (it == functions.end())
					wpp::error(positions[node_id], env, "func not found: ", caller_name, ".");

				if (it->second.empty())
					wpp::error(positions[node_id], env, "func not found: ", caller_name, ".");

				const auto func = ast.get<wpp::Fn>(it->second.back());

				// Retrieve function.
				const auto& [callee_name, params, body] = func;

				// Set up Arguments to pass down to function body.
				wpp::FnEnv new_fn_env;

				if (fn_env)
					new_fn_env.args = fn_env->args;

				// Evaluate arguments and store their result.
				for (int i = 0; i < (int)caller_args.size(); i++) {
					const auto result = evaluate(caller_args[i], env, fn_env);

					if (auto it = new_fn_env.args.find(params[i]); it != new_fn_env.args.end()) {
						if (warning_flags & wpp::WARN_PARAM_SHADOW_PARAM)
							wpp::warn(positions[node_id], env,
								"parameter '", it->first, "' inside function '", callee_name, "' shadows parameter from parent scope."
							);

						it->second = result;
					}

					else
						new_fn_env.args.insert_or_assign(params[i], evaluate(caller_args[i], env, fn_env));
				}

				// Call function.
				str = evaluate(body, env, &new_fn_env);
			},

			[&] (const Fn& func) {
				wpp::dbg("(eval) func");

				auto& [ast, functions, positions, root, warning_flags, sources] = env;
				const auto& [name, params, body] = func;

				auto it = functions.find(wpp::cat(name, params.size()));

				if (it != functions.end()) {
					if (warning_flags & wpp::WARN_FUNC_REDEFINED)
						wpp::warn(positions[node_id], env, "function '", name, "' redefined.");

					it->second.emplace_back(node_id);
				}

				else
					functions.emplace(wpp::cat(name, params.size()), std::vector{node_id});
			},

			[&] (const Codeify& colby) {
				wpp::dbg("(eval) codeify");

				str = wpp::intrinsic_eval(node_id, {colby.expr}, env, fn_env);
			},

			[&] (const Var& var) {
				wpp::dbg("(eval) var");

				auto& [ast, functions, positions, root, warning_flags, sources] = env;
				auto& [name, body] = var;

				const auto func_name = wpp::cat(name, 0);
				const auto str = evaluate(body, env, fn_env);

				// Replace body with a string of the evaluation result.
				ast.replace<String>(body, str);

				// Replace Var node with Fn node.
				ast.replace<Fn>(node_id, name, std::vector<wpp::View>{}, body);

				auto it = functions.find(func_name);

				if (it != functions.end()) {
					if (warning_flags & wpp::WARN_VARFUNC_REDEFINED)
						wpp::warn(positions[node_id], env, "function/variable '", name, "' redefined.");

					it->second.emplace_back(node_id);
				}

				else
					functions.emplace(func_name, std::vector{node_id});
			},

			[&] (const Drop& drop) {
				wpp::dbg("(eval) drop");

				auto& [ast, functions, positions, root, warning_flags, sources] = env;
				const auto& [func_id] = drop;

				auto* func = std::get_if<FnInvoke>(&ast[func_id]);

				if (not func)
					wpp::error(positions[node_id], env, "invalid function passed to drop.");

				const auto& [caller_name, caller_args] = *func;

				std::string caller_mangled_name = wpp::cat(caller_name, caller_args.size());

				auto it = functions.find(caller_mangled_name);

				if (it != functions.end()) {
					if (not it->second.empty())
						it->second.pop_back();

					if (it->second.empty())
						functions.erase(it);
				}

				else {
					wpp::error(positions[node_id], env, "cannot drop undefined function '", caller_name, "' (", caller_args.size(), " parameters).");
				}
			},

			[&] (const String& x) {
				wpp::dbg("(eval) string");
				str = x.value;
			},

			[&] (const Concat& cat) {
				wpp::dbg("(eval) cat");

				const auto& [lhs, rhs] = cat;
				str = evaluate(lhs, env, fn_env) + evaluate(rhs, env, fn_env);
			},

			[&] (const Block& block) {
				wpp::dbg("(eval) block");
				const auto& [stmts, expr] = block;

				for (const wpp::node_t node: stmts)
					str += evaluate(node, env, fn_env);

				str = evaluate(expr, env, fn_env);
			},

			[&] (const Map& map) {
				wpp::dbg("(eval) map");

				auto& [ast, functions, positions, root, warning_flags, sources] = env;
				const auto& [test, cases, default_case] = map;

				const auto test_str = evaluate(test, env, fn_env);

				// Compare test_str with arms of the map.
				auto it = std::find_if(cases.begin(), cases.end(), [&] (const auto& elem) {
					return test_str == evaluate(elem.first, env, fn_env);
				});

				// If found, evaluate the hand.
				if (it != cases.end())
					str = evaluate(it->second, env, fn_env);

				// If not found, check for a default arm, otherwise error.
				else {
					if (default_case == wpp::NODE_EMPTY)
						wpp::error(positions[node_id], env, "no matches found.");

					else
						str = evaluate(default_case, env, fn_env);
				}
			},

			[&] (const Pre&) {
				wpp::dbg("(eval) prefix");

				// auto& [ast, functions, positions, root, warning_flags, sources] = env;
				// const auto& [exprs, stmts] = pre;

				// for (const wpp::node_t stmt: stmts) {
				// 	if (wpp::Fn* func = std::get_if<wpp::Fn>(&ast[stmt])) {
				// 		std::string name;

				// 		for (auto it = exprs.rbegin(); it != exprs.rend(); ++it)
				// 			name += evaluate(*it, env, fn_env);

				// 		func->identifier = name + func->identifier;
				// 		str += evaluate(stmt, env, fn_env);
				// 	}

				// 	else if (wpp::Pre* pre = std::get_if<wpp::Pre>(&ast[stmt])) {
				// 		pre->exprs.insert(pre->exprs.end(), exprs.begin(), exprs.end());
				// 		str += evaluate(stmt, env, fn_env);
				// 	}

				// 	else {
				// 		str += evaluate(stmt, env, fn_env);
				// 	}
				// }
			},

			[&] (const Document& doc) {
				if (env.flags & wpp::INTERNAL_ERROR_STATE)
					throw wpp::Error{};

				wpp::dbg("(eval) document");

				for (const wpp::node_t node: doc.stmts)
					str += evaluate(node, env, fn_env);
			}
		);

		return str;
	}
}

