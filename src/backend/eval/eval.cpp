#include <string>
#include <vector>
#include <iterator>
#include <algorithm>

#include <misc/util/util.hpp>
#include <misc/flags.hpp>
#include <structures/environment.hpp>
#include <frontend/lexer/lexer.hpp>
#include <frontend/parser/ast_nodes.hpp>
#include <backend/eval/intrinsics.hpp>


namespace wpp {
	std::string evaluate(const wpp::node_t, wpp::Env&, wpp::FnEnv*);

	namespace {
		std::string eval_intrinsic(wpp::node_t, const Intrinsic&, wpp::Env&, wpp::FnEnv*);
		std::string eval_fninvoke(wpp::node_t, const FnInvoke&, wpp::Env&, wpp::FnEnv*);
		std::string eval_fn(wpp::node_t, const Fn&, wpp::Env&, wpp::FnEnv*);
		std::string eval_codeify(wpp::node_t, const Codeify&, wpp::Env&, wpp::FnEnv*);
		std::string eval_varref(wpp::node_t, const VarRef&, wpp::Env&, wpp::FnEnv*);
		std::string eval_var(wpp::node_t, const Var&, wpp::Env&, wpp::FnEnv*);
		std::string eval_push(wpp::node_t, const Push&, wpp::Env&, wpp::FnEnv*);
		std::string eval_pop(wpp::node_t, const Pop&, wpp::Env&, wpp::FnEnv*);
		std::string eval_use(wpp::node_t, const Use&, wpp::Env&, wpp::FnEnv*);
		std::string eval_drop(wpp::node_t, const Drop&, wpp::Env&, wpp::FnEnv*);
		std::string eval_string(wpp::node_t, const String&, wpp::Env&, wpp::FnEnv*);
		std::string eval_cat(wpp::node_t, const Concat&, wpp::Env&, wpp::FnEnv*);
		std::string eval_block(wpp::node_t, const Block&, wpp::Env&, wpp::FnEnv*);
		std::string eval_map(wpp::node_t, const Map&, wpp::Env&, wpp::FnEnv*);
		std::string eval_document(wpp::node_t, const Document&, wpp::Env&, wpp::FnEnv*);
	}
}


namespace wpp { namespace {
	std::string eval_intrinsic(wpp::node_t node_id, const Intrinsic& fn, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG("intrinsic");
		std::string str;

		const auto& type = fn.type;
		const auto& name = fn.identifier;
		const auto& exprs = fn.arguments;

		#define INTRINSIC(n, fn, tok) \
			if (type == tok) { \
				if (n != exprs.size()) \
					wpp::error(node_id, env, "incorrect argument count", wpp::cat("intrinsic '", name, "' takes exactly ", n, " arguments")); \
				str = wpp::intrinsic_##fn(node_id, exprs, env, fn_env); \
				return str; \
			}

		INTRINSIC(3, slice,  TOKEN_SLICE);
		INTRINSIC(2, find,   TOKEN_FIND);
		INTRINSIC(2, assert, TOKEN_ASSERT);
		INTRINSIC(2, pipe,   TOKEN_PIPE);
		INTRINSIC(1, error,  TOKEN_ERROR);
		INTRINSIC(1, file,   TOKEN_FILE);
		INTRINSIC(1, escape, TOKEN_ESCAPE);
		INTRINSIC(1, run,    TOKEN_RUN);
		INTRINSIC(1, length, TOKEN_LENGTH);
		INTRINSIC(1, log,    TOKEN_LOG);

		#undef INTRINSIC

		return str;
	}


	std::string eval_fninvoke(wpp::node_t node_id, const FnInvoke& call, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG("fninvoke");

		auto& functions = env.functions;
		const auto& ast = env.ast;
		const auto& flags = env.flags;

		const auto& caller_name = call.identifier;
		const auto& caller_args = call.arguments;

		std::string caller_mangled_name = wpp::cat(caller_name, caller_args.size());


		auto func_it = functions.find(caller_mangled_name);

		if (func_it == functions.end() or func_it->second.empty()) {
			func_it = functions.find(wpp::cat(caller_name, "_v"));

			if (func_it == functions.end() or func_it->second.empty())
				wpp::error(
					node_id, env,
					"function not found",
					wpp::cat("attempting to invoke function '", caller_name, "' (", caller_args.size(), " parameters) which is undefined"),
					"are you passing the correct number of arguments?"
				);
		}

		const auto func = ast.get<wpp::Fn>(func_it->second.back());

		// Retrieve function.
		const auto& callee_name = func.identifier;
		const auto& params = func.parameters;
		const auto body = func.body;
		const auto is_variadic = func.is_variadic;


		// Set up Arguments to pass down to function body.
		wpp::FnEnv new_fn_env;

		if (fn_env)
			new_fn_env.arguments = fn_env->arguments;


		// Handle variadic args.
		for (auto it = caller_args.rbegin(); it != (caller_args.rend() - params.size()); ++it) {
			const auto result = evaluate(*it, env, fn_env);
			eval_push(node_id, wpp::Push{*it}, env, fn_env);
		}

		// Evaluate arguments and store their result.
		for (auto it = params.rbegin(); it != params.rend(); ++it) {
			// Calculate distance from end of params to current iterator -1
			const auto index = std::distance(it, params.rend()) - 1;
			const auto result = evaluate(caller_args[index], env, fn_env);

			// If parameter is not already in environment, insert it.
			if (const auto arg_it = new_fn_env.arguments.find(*it); arg_it == new_fn_env.arguments.end())
				new_fn_env.arguments.emplace(*it, result);

			// If parameter exists, overwrite it.
			else {
				arg_it->second = result;

				if (flags & wpp::WARN_PARAM_SHADOW_PARAM)
					wpp::warn(node_id, env, "parameter shadows parameter",
						wpp::cat("parameter '", arg_it->first, "' inside function '", callee_name, "' shadows parameter from enclosing function")
					);
			}
		}


		// Call function.
		return evaluate(body, env, &new_fn_env);
	}


	std::string eval_fn(wpp::node_t node_id, const Fn& func, wpp::Env& env, wpp::FnEnv* fn_env) {
		auto& functions = env.functions;
		const auto& flags = env.flags;

		const auto& name = func.identifier;
		const auto& params = func.parameters;
		const auto& body = func.body;
		const auto& is_variadic = func.is_variadic;

		DBG("fn: ", name, ", body: ", body);


		std::string mangled_name;

		if (is_variadic)
			mangled_name = wpp::cat(name, "_v");

		else
			mangled_name = wpp::cat(name, params.size());


		if (auto it = functions.find(mangled_name); it != functions.end()) {
			if (flags & wpp::WARN_FUNC_REDEFINED)
				wpp::warn(node_id, env, "function redefined", wpp::cat("function '", name, "' redefined"));

			it->second.emplace_back(node_id);
		}

		else
			functions.emplace(mangled_name, std::vector{node_id});

		return "";
	}


	std::string eval_codeify(wpp::node_t node_id, const Codeify& colby, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG("codeify");
		return wpp::intrinsic_eval(node_id, {colby.expr}, env, fn_env);
	}


	std::string eval_varref(wpp::node_t node_id, const VarRef& varref, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG("varref");
		std::string str;

		const auto& flags = env.flags;
		auto& variables = env.variables;

		const auto& name = varref.identifier;

		// Check if parameter.
		if (fn_env) {
			auto& arguments = fn_env->arguments;

			if (auto it = arguments.find(name); it != arguments.end()) {
				str = it->second; // Return str.

				// Check if it's shadowing a function (even this one).
				if (flags & wpp::WARN_PARAM_SHADOW_VAR and variables.find(name.str()) != variables.end())
					wpp::warn(
						node_id, env,
						"parameter shadows variable",
						wpp::cat("parameter '", name.str(), "' is shadowing a variable")
					);

				return str;
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

		return str;
	}


	std::string eval_var(wpp::node_t node_id, const Var& var, wpp::Env& env, wpp::FnEnv* fn_env) {
		std::string str;

		const auto& flags = env.flags;
		auto& variables = env.variables;

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

		return str;
	}


	std::string eval_push(wpp::node_t node_id, const Push& psh, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG("push");
		env.stack.emplace(evaluate(psh.expr, env, fn_env));
		return "";
	}


	std::string eval_pop(wpp::node_t node_id, const Pop& pop, wpp::Env& env, wpp::FnEnv* fn_env) {
		std::string str;

		auto& stack = env.stack;
		auto& functions = env.functions;
		auto& ast = env.ast;

		const auto& func = pop.identifier;
		const auto index = pop.index_of_popped_arg;
		auto args = pop.arguments;

		DBG();

		if (not stack.empty()) {
			std::string top = stack.top();
			wpp::node_t node = ast.add<String>(top);
			stack.pop();

			args[index] = node;

			str = eval_fninvoke(node_id, FnInvoke{args, func}, env, fn_env);
		}

		else {
			args.erase(args.begin() + index);
			str = eval_fninvoke(node_id, FnInvoke{args, func}, env, fn_env);
		}

		DBG("pop: '", str, "'");

		return str;
	}


	std::string eval_use(wpp::node_t node_id, const Use& use, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG("use: ", evaluate(use.path, env, fn_env));
		return wpp::intrinsic_source(node_id, {use.path}, env, fn_env);
	}


	std::string eval_drop(wpp::node_t node_id, const Drop& drop, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG("drop");
		std::string str;

		const auto& flags = env.flags;
		const auto& ast = env.ast;
		auto& functions = env.functions;

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

		return str;
	}


	std::string eval_string(wpp::node_t node_id, const String& str, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG("string");
		return str.value;
	}


	std::string eval_cat(wpp::node_t node_id, const Concat& cat, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG("cat");
		return evaluate(cat.lhs, env, fn_env) + evaluate(cat.rhs, env, fn_env);
	}


	std::string eval_block(wpp::node_t node_id, const Block& block, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG("block");

		for (const wpp::node_t node: block.statements)
			evaluate(node, env, fn_env);

		return evaluate(block.expr, env, fn_env);
	}


	std::string eval_map(wpp::node_t node_id, const Map& map, wpp::Env& env, wpp::FnEnv* fn_env) {
		std::string str;

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

		return str;
	}


	std::string eval_document(wpp::node_t node_id, const Document& doc, wpp::Env& env, wpp::FnEnv* fn_env) {
		std::string str;

		if (env.state & wpp::INTERNAL_ERROR_STATE)
			throw wpp::Error{};

		DBG("doc");

		for (const wpp::node_t node: doc.statements)
			str += evaluate(node, env, fn_env);

		return str;
	}
}}


namespace wpp {
	// The core of the evaluator.
	std::string evaluate(const wpp::node_t node_id, wpp::Env& env, wpp::FnEnv* fn_env) {
		const auto& variant = env.ast[node_id];

		return wpp::visit(env.ast[node_id],
			[&] (const Intrinsic& x) { return eval_intrinsic (node_id, x, env, fn_env); },
			[&] (const FnInvoke& x)  { return eval_fninvoke  (node_id, x, env, fn_env); },
			[&] (const Fn& x)        { return eval_fn        (node_id, x, env, fn_env); },
			[&] (const Codeify& x)   { return eval_codeify   (node_id, x, env, fn_env); },
			[&] (const VarRef& x)    { return eval_varref    (node_id, x, env, fn_env); },
			[&] (const Var& x)       { return eval_var       (node_id, x, env, fn_env); },
			[&] (const Push& x)      { return eval_push      (node_id, x, env, fn_env); },
			[&] (const Pop& x)       { return eval_pop       (node_id, x, env, fn_env); },
			[&] (const Use& x)       { return eval_use       (node_id, x, env, fn_env); },
			[&] (const Drop& x)      { return eval_drop      (node_id, x, env, fn_env); },
			[&] (const String& x)    { return eval_string    (node_id, x, env, fn_env); },
			[&] (const Concat& x)    { return eval_cat       (node_id, x, env, fn_env); },
			[&] (const Block& x)     { return eval_block     (node_id, x, env, fn_env); },
			[&] (const Map& x)       { return eval_map       (node_id, x, env, fn_env); },
			[&] (const Document& x)  { return eval_document  (node_id, x, env, fn_env); }
		);
	}
}

