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
		std::string eval_dropfunc(wpp::node_t, const DropFunc&, wpp::Env&, wpp::FnEnv*);
		std::string eval_dropvarfunc(wpp::node_t, const DropVarFunc&, wpp::Env&, wpp::FnEnv*);
		std::string eval_dropvar(wpp::node_t, const DropVar&, wpp::Env&, wpp::FnEnv*);
		std::string eval_string(wpp::node_t, const String&, wpp::Env&, wpp::FnEnv*);
		std::string eval_cat(wpp::node_t, const Concat&, wpp::Env&, wpp::FnEnv*);
		std::string eval_block(wpp::node_t, const Block&, wpp::Env&, wpp::FnEnv*);
		std::string eval_map(wpp::node_t, const Map&, wpp::Env&, wpp::FnEnv*);
		std::string eval_document(wpp::node_t, const Document&, wpp::Env&, wpp::FnEnv*);
	}
}


// Utils
namespace wpp { namespace {
	wpp::Fn find_func(wpp::node_t node_id, const FnInvoke& call, int n_args, wpp::Env& env) {
		auto& functions = env.functions;
		auto& variadic_functions = env.variadic_functions;
		const auto& ast = env.ast;
		const auto& flags = env.flags;

		const auto& name = call.identifier;
		const auto& args = call.arguments;

		wpp::Fn func{};

		// Lookup normal function first.
		const auto it = functions.find(wpp::FuncKey{name, n_args});

		if (it != functions.end())
			func = ast.get<wpp::Fn>(it->second.back());

		// No normal function found, we look up a variadic function.
		else {
			const auto it = variadic_functions.find(name);

			// Check if we found a match and that the caller has the minimum number of arguments required.
			if (it != variadic_functions.end() and n_args >= it->second.min_args)
				func = ast.get<wpp::Fn>(it->second.generations.back());

			// No func found, normal nor variadic.
			else
				wpp::error(node_id, env, "function not found",
					wpp::cat("attempting to invoke function '", name, "' (", n_args, " parameters) which is undefined"),
					"are you passing the correct number of arguments?"
				);
		}

		return func;
	}


	std::string call_func(wpp::node_t node_id, const FnInvoke& call, const std::vector<std::string>& arg_strings, wpp::Env& env, wpp::FnEnv* fn_env) {
		auto& functions = env.functions;
		auto& variadic_functions = env.variadic_functions;
		const auto& ast = env.ast;
		const auto& flags = env.flags;

		const auto& name = call.identifier;
		const auto& args = call.arguments;


		wpp::Fn func = find_func(node_id, call, arg_strings.size(), env);


		// Set up Arguments to pass down to function body.
		wpp::FnEnv new_fn_env;

		if (fn_env)
			new_fn_env.arguments = fn_env->arguments;


		const auto& params = func.parameters;

		// Push variadic arguments.
		for (auto it = arg_strings.rbegin(); it != (arg_strings.rend() - params.size()); ++it)
			env.stack.emplace(*it);


		// Setup normal arguments.
		for (auto it = params.rbegin(); it != params.rend(); ++it) {
			// Calculate distance from end of params to current iterator -1
			const auto index = std::distance(it, params.rend()) - 1;
			const auto& result = arg_strings[index];

			const auto arg_it = new_fn_env.arguments.find(*it);

			// If parameter is not already in environment, insert it.
			if (arg_it == new_fn_env.arguments.end())
				new_fn_env.arguments.emplace(*it, result);

			// If parameter exists, overwrite it.
			else {
				arg_it->second = result;

				if (flags & wpp::WARN_PARAM_SHADOW_PARAM)
					wpp::warn(node_id, env, "parameter shadows parameter",
						wpp::cat("parameter '", arg_it->first, "' inside function '", name, "' shadows parameter from enclosing function")
					);
			}
		}


		// Call function.
		env.call_depth++;

		if (flags & wpp::WARN_DEEP_RECURSION and env.call_depth % 128 == 0)
			wpp::warn(node_id, env, "deep recursion", wpp::cat("the call stack has grown to a depth of ", env.call_depth),
				"a large call depth may indicate recursion without an exit condition"
			);

		const std::string str = evaluate(func.body, env, &new_fn_env);
		env.call_depth--;

		return str;
	}
}}


namespace wpp { namespace {
	std::string eval_intrinsic(wpp::node_t node_id, const Intrinsic& fn, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
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

		INTRINSIC(2, assert, TOKEN_ASSERT);
		INTRINSIC(2, pipe,   TOKEN_PIPE);
		INTRINSIC(1, error,  TOKEN_ERROR);
		INTRINSIC(1, file,   TOKEN_FILE);
		INTRINSIC(1, escape, TOKEN_ESCAPE);
		INTRINSIC(1, run,    TOKEN_RUN);
		INTRINSIC(1, log,    TOKEN_LOG);

		#undef INTRINSIC

		return str;
	}


	std::string eval_fninvoke(wpp::node_t node_id, const FnInvoke& call, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		// Evaluate arguments.
		std::vector<std::string> arg_strings;

		for (const wpp::node_t node: call.arguments)
			arg_strings.emplace_back(wpp::evaluate(node, env, fn_env));

		return wpp::call_func(node_id, call, arg_strings, env, fn_env);
	}


	std::string eval_fn(wpp::node_t node_id, const Fn& func, wpp::Env& env, wpp::FnEnv* fn_env) {
		auto& functions = env.functions;
		auto& variadic_functions = env.variadic_functions;
		const auto& flags = env.flags;

		const auto& name = func.identifier;
		const auto& params = func.parameters;
		const auto& is_variadic = func.is_variadic;

		DBG("fn: ", name);


		if (is_variadic) {
			if (auto it = variadic_functions.find(name); it != variadic_functions.end()) {
				if (flags & wpp::WARN_FUNC_REDEFINED)
					wpp::warn(node_id, env, "variadic function redefined", wpp::cat("variadic function '", name, "' redefined"));

				it->second.generations.emplace_back(node_id);
			}

			else
				variadic_functions.emplace(name, wpp::VariadicFuncEntry{std::vector{node_id}, params.size()});
		}

		else {
			const wpp::FuncKey key{name, params.size()};

			if (auto it = functions.find(key); it != functions.end()) {
				if (flags & wpp::WARN_FUNC_REDEFINED)
					wpp::warn(node_id, env, "function redefined", wpp::cat("function '", name, "' redefined"));

				it->second.emplace_back(node_id);
			}

			else
				functions.emplace(key, std::vector{node_id});
		}


		return "";
	}


	std::string eval_codeify(wpp::node_t node_id, const Codeify& colby, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		return wpp::intrinsic_eval(node_id, {colby.expr}, env, fn_env);
	}


	std::string eval_varref(wpp::node_t node_id, const VarRef& varref, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		const auto& flags = env.flags;
		auto& variables = env.variables;

		const auto& name = varref.identifier;


		// Check if parameter.
		if (fn_env)
			if (const auto it = fn_env->arguments.find(name); it != fn_env->arguments.end()) {
				// Check if it's shadowing a variable.
				if (flags & wpp::WARN_PARAM_SHADOW_VAR and variables.find(name) != variables.end())
					wpp::warn(node_id, env, "parameter shadows variable", wpp::cat("parameter '", name.str(), "' is shadowing a variable"));

				return it->second; // Return str.
			}


		// Check if variable.
		if (const auto it = variables.find(name); it != variables.end())
			return it->second.back();

		wpp::error(node_id, env, "variable not found",
			wpp::cat("attempting to reference variable '", name.str(), "' which is undefined")
		);

		return "";
	}


	std::string eval_var(wpp::node_t node_id, const Var& var, wpp::Env& env, wpp::FnEnv* fn_env) {
		const auto& flags = env.flags;
		auto& variables = env.variables;

		const auto name = var.identifier;
		DBG("var: ", name);


		if (auto it = variables.find(name); it != variables.end()) {
			if (flags & wpp::WARN_VAR_REDEFINED)
				wpp::warn(node_id, env, "variable redefined", wpp::cat("variable '", name, "' redefined"));

			it->second.emplace_back(wpp::evaluate(var.body, env, fn_env));
		}

		else
			variables.emplace(name, std::vector{wpp::evaluate(var.body, env, fn_env)});

		return "";
	}


	std::string eval_push(wpp::node_t node_id, const Push& psh, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		env.stack.emplace(evaluate(psh.expr, env, fn_env));
		return "";
	}


	std::string eval_pop(wpp::node_t node_id, const Pop& pop, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		auto& stack = env.stack;
		auto& functions = env.functions;
		auto& ast = env.ast;

		const auto& func = pop.identifier;
		const auto n_popped_args = pop.n_popped_args;
		const auto& args = pop.arguments;


		// Evaluate arguments.
		std::vector<std::string> arg_strings;

		for (const wpp::node_t node: args)
			arg_strings.emplace_back(wpp::evaluate(node, env, fn_env));


		// Loop to collect as many strings from the stack as possible until we reach `n_popped_args`
		// or the stack is empty.
		for (int i = 0; not stack.empty() and i < n_popped_args; ++i) {
			arg_strings.emplace_back(stack.top());
			stack.pop();
		}

		return wpp::call_func(node_id, FnInvoke{args, func}, arg_strings, env, fn_env);
	}


	std::string eval_use(wpp::node_t node_id, const Use& use, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG("use: ", evaluate(use.path, env, fn_env));
		return wpp::intrinsic_source(node_id, {use.path}, env, fn_env);
	}


	std::string eval_dropfunc(wpp::node_t node_id, const DropFunc& drop, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		auto& functions = env.functions;

		if (auto it = functions.find(wpp::FuncKey{drop.identifier, drop.n_args}); it != functions.end()) {
			if (not it->second.empty())
				it->second.pop_back();

			if (it->second.empty())
				functions.erase(it);
		}

		else
			wpp::error(node_id, env, "undefined function",
				wpp::cat("cannot drop undefined function '", drop.identifier, "' (", drop.n_args, " parameters)"),
				"are you passing the correct number of arguments?"
			);

		return "";
	}


	std::string eval_dropvarfunc(wpp::node_t node_id, const DropVarFunc& drop, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		auto& variadic_functions = env.variadic_functions;

		const auto& name = drop.identifier;
		const auto drop_min_args = drop.min_args;

		if (auto it = variadic_functions.find(name); it != variadic_functions.end()) {
			auto& [generations, min_args] = it->second;

			if (drop_min_args == min_args) {
				if (not generations.empty())
					generations.pop_back();

				if (generations.empty())
					variadic_functions.erase(it);

				return "";
			}
		}

		wpp::error(node_id, env, "undefined variadic function",
			wpp::cat("cannot drop undefined variadic function '", name, "' (>=", drop_min_args, " parameters, variadic)"),
			"are you passing the correct number of arguments?"
		);
	}


	std::string eval_dropvar(wpp::node_t node_id, const DropVar& drop, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		auto& variables = env.variables;

		if (auto it = variables.find(drop.identifier); it != variables.end()) {
			if (not it->second.empty())
				it->second.pop_back();

			if (it->second.empty())
				variables.erase(it);
		}

		else
			wpp::error(node_id, env, "undefined variables", wpp::cat("cannot drop undefined variable '", drop.identifier, "'"));

		return "";
	}


	std::string eval_string(wpp::node_t node_id, const String& str, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		return str.value;
	}


	std::string eval_cat(wpp::node_t node_id, const Concat& cat, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		return evaluate(cat.lhs, env, fn_env) + evaluate(cat.rhs, env, fn_env);
	}


	std::string eval_block(wpp::node_t node_id, const Block& block, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		for (const wpp::node_t node: block.statements)
			evaluate(node, env, fn_env);

		return evaluate(block.expr, env, fn_env);
	}


	std::string eval_map(wpp::node_t node_id, const Map& map, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		std::string str;

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

		DBG();

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
			[&] (const Intrinsic& x)   { return eval_intrinsic    (node_id, x, env, fn_env); },
			[&] (const FnInvoke& x)    { return eval_fninvoke     (node_id, x, env, fn_env); },
			[&] (const Fn& x)          { return eval_fn           (node_id, x, env, fn_env); },
			[&] (const Codeify& x)     { return eval_codeify      (node_id, x, env, fn_env); },
			[&] (const VarRef& x)      { return eval_varref       (node_id, x, env, fn_env); },
			[&] (const Var& x)         { return eval_var          (node_id, x, env, fn_env); },
			[&] (const Push& x)        { return eval_push         (node_id, x, env, fn_env); },
			[&] (const Pop& x)         { return eval_pop          (node_id, x, env, fn_env); },
			[&] (const Use& x)         { return eval_use          (node_id, x, env, fn_env); },
			[&] (const DropFunc& x)    { return eval_dropfunc     (node_id, x, env, fn_env); },
			[&] (const DropVarFunc& x) { return eval_dropvarfunc  (node_id, x, env, fn_env); },
			[&] (const DropVar& x)     { return eval_dropvar      (node_id, x, env, fn_env); },
			[&] (const String& x)      { return eval_string       (node_id, x, env, fn_env); },
			[&] (const Concat& x)      { return eval_cat          (node_id, x, env, fn_env); },
			[&] (const Block& x)       { return eval_block        (node_id, x, env, fn_env); },
			[&] (const Map& x)         { return eval_map          (node_id, x, env, fn_env); },
			[&] (const Document& x)    { return eval_document     (node_id, x, env, fn_env); }
		);
	}
}

