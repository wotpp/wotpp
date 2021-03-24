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
		const auto& ast = env.ast;
		auto& functions = env.functions;
		auto& variables = env.variables;
		auto& stack = env.stack;
		const auto& positions = env.positions;
		const auto& flags = env.flags;

		const auto& variant = env.ast[node_id];
		std::string str;

		wpp::visit(variant,
			[&] (const Intrinsic& fn) {
				DBG("intrinsic");

				const auto& type = fn.type;
				const auto& name = fn.identifier;
				const auto& exprs = fn.arguments;

				#define INTRINSIC(n, fn, tok) \
					if (type == tok) { \
						if (n != exprs.size()) \
							wpp::error(node_id, env, "incorrect argument count", wpp::cat("intrinsic '", name, "' takes exactly ", n, " arguments")); \
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
				INTRINSIC(1, run,    TOKEN_RUN);
				INTRINSIC(1, source, TOKEN_SOURCE);
				INTRINSIC(1, length, TOKEN_LENGTH);
				INTRINSIC(1, log,    TOKEN_LOG);

				#undef INTRINSIC
			},

			[&] (const FnInvoke& call) {
				DBG("fninvoke");

				const auto& caller_name = call.identifier;
				const auto& caller_args = call.arguments;

				std::string caller_mangled_name = wpp::cat(caller_name, caller_args.size());

				// If it wasn't a parameter, we fall through to here and check if it's a function.
				auto func_it = functions.find(caller_mangled_name);

				if (func_it == functions.end() or func_it->second.empty())
					wpp::error(
						node_id, env,
						"function not found",
						wpp::cat("attempting to invoke function '", caller_name, "' which is undefined"),
						"are you passing the correct number of arguments?"
					);

				const auto func = ast.get<wpp::Fn>(func_it->second.back());

				// Retrieve function.
				const auto& callee_name = func.identifier;
				const auto& params = func.parameters;
				const auto& body = func.body;

				// Set up Arguments to pass down to function body.
				wpp::FnEnv new_fn_env;

				if (fn_env)
					new_fn_env.args = fn_env->args;

				// Evaluate arguments and store their result.
				for (int i = 0; i < (int)caller_args.size(); i++) {
					const auto result = evaluate(caller_args[i], env, fn_env);

					if (auto it = new_fn_env.args.find(params[i]); it != new_fn_env.args.end()) {
						if (flags & wpp::WARN_PARAM_SHADOW_PARAM)
							wpp::warn(
								node_id, env,
								"parameter shadows parameter",
								wpp::cat(
									"parameter '", it->first, "' inside function '", callee_name, "' shadows parameter from enclosing function"
								)
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
				const auto& name = func.identifier;
				const auto& params = func.parameters;
				const auto& body = func.body;

				DBG("fn: ", name, ", body: ", body);

				auto it = functions.find(wpp::cat(name, params.size()));

				if (it != functions.end()) {
					if (flags & wpp::WARN_FUNC_REDEFINED)
						wpp::warn(
							node_id, env,
							"function redefined",
							wpp::cat("function '", name, "' redefined")
						);

					it->second.emplace_back(node_id);
				}

				else
					functions.emplace(wpp::cat(name, params.size()), std::vector{node_id});
			},

			[&] (const Codeify& colby) {
				DBG("codeify");
				str = wpp::intrinsic_eval(node_id, {colby.expr}, env, fn_env);
			},

			[&] (const VarRef& varref) {
				DBG("varref");

				const auto& name = varref.identifier;

				// Check if parameter.
				if (fn_env) {
					auto& args = fn_env->args;

					if (auto it = args.find(name); it != args.end()) {
						str = it->second; // Return str.

						// Check if it's shadowing a function (even this one).
						if (flags & wpp::WARN_PARAM_SHADOW_VAR and variables.find(name.str()) != variables.end())
							wpp::warn(
								node_id, env,
								"parameter shadows variable",
								wpp::cat("parameter '", name.str(), "' is shadowing a variable")
							);

						return;
					}
				}

				if (auto it = variables.find(name.str()); it != variables.end())
					str = it->second;

				else
					wpp::error(
						node_id, env,
						"variable not found",
						wpp::cat("attempting to reference variable '", name.str(), "' which is undefined")
					);
			},

			[&] (const Var& var) {
				const auto name = var.identifier.str();
				const auto& body = var.body;

				DBG("var: ", name, ", body: ", body);

				if (auto it = variables.find(name); it != variables.end()) {
					if (flags & wpp::WARN_VAR_REDEFINED)
						wpp::warn(
							node_id, env,
							"variable redefined",
							wpp::cat("variable '", name, "' redefined")
						);

					it->second = evaluate(body, env, fn_env);
				}

				else
					variables.emplace(name, evaluate(body, env, fn_env));
			},

			[&] (const Push& psh) {
				DBG("push");
				stack.emplace(evaluate(psh.expr, env, fn_env));
			},

			[&] (const Pop& poop) {
				const wpp::node_t expr = poop.expr;

				if (not stack.empty()) {
					str = stack.top();
					stack.pop();
					evaluate(expr, env, fn_env);
				}

				else {
					// wpp::error(node_id, env, "stack underflow", "popping from an empty stack");
				}

				DBG("pop: '", str, "'");
			},

			[&] (const Use& use) {
				DBG("use: ", evaluate(use.path, env, fn_env));
				str = wpp::intrinsic_source(node_id, {use.path}, env, fn_env);
			},

			[&] (const Drop& drop) {
				DBG("drop");

				const auto& func_id = drop.func;

				auto* func = std::get_if<FnInvoke>(&ast[func_id]);

				if (not func)
					wpp::error(
						node_id, env,
						"expected function invocation",
						"expecting a function invocation after `drop`"
					);

				const auto& caller_name = func->identifier;
				const auto& caller_args = func->arguments;

				std::string caller_mangled_name = wpp::cat(caller_name, caller_args.size());

				auto it = functions.find(caller_mangled_name);

				if (it != functions.end()) {
					if (not it->second.empty())
						it->second.pop_back();

					if (it->second.empty())
						functions.erase(it);
				}

				else {
					wpp::error(
						node_id, env,
						"undefined function",
						wpp::cat("cannot drop undefined function '", caller_name, "' (", caller_args.size(), " parameters)"),
						"are you passing the correct number of arguments?"
					);
				}
			},

			[&] (const String& x) {
				DBG("string");
				str = x.value;
			},

			[&] (const Concat& cat) {
				DBG("cat");
				str = evaluate(cat.lhs, env, fn_env) + evaluate(cat.rhs, env, fn_env);
			},

			[&] (const Block& block) {
				DBG("block");

				for (const wpp::node_t node: block.statements)
					str += evaluate(node, env, fn_env);

				str = evaluate(block.expr, env, fn_env);
			},

			[&] (const Map& map) {
				DBG("map");

				const auto& test = map.expr;
				const auto& cases = map.cases;
				const auto& default_case = map.default_case;

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
						wpp::error(
							node_id, env,
							"no matches found",
							"exhausted all checks in map expression"
						);

					else
						str = evaluate(default_case, env, fn_env);
				}
			},

			// [&] (const Pre&) {
				// DBG();

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
			// },

			[&] (const Document& doc) {
				if (env.state & wpp::INTERNAL_ERROR_STATE)
					throw wpp::Error{};

				DBG("doc");

				for (const wpp::node_t node: doc.statements)
					str += evaluate(node, env, fn_env);
			}
		);

		return str;
	}
}

