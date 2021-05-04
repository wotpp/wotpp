#include <string>
#include <vector>
#include <iterator>
#include <algorithm>
#include <functional>

#include <misc/util/util.hpp>
#include <misc/flags.hpp>
#include <structures/environment.hpp>
#include <frontend/lexer/lexer.hpp>
#include <frontend/parser/ast_nodes.hpp>
#include <backend/eval/intrinsics.hpp>


namespace wpp {
	std::string evaluate(const wpp::node_t, wpp::Env&, wpp::FnEnv*);

	namespace {
		std::string eval_intrinsic_run(wpp::node_t, const FnInvoke&, wpp::Env&, wpp::FnEnv*);
		std::string eval_intrinsic_pipe(wpp::node_t, const FnInvoke&, wpp::Env&, wpp::FnEnv*);
		std::string eval_intrinsic_log(wpp::node_t, const FnInvoke&, wpp::Env&, wpp::FnEnv*);
		std::string eval_intrinsic_error(wpp::node_t, const FnInvoke&, wpp::Env&, wpp::FnEnv*);
		std::string eval_intrinsic_assert(wpp::node_t, const FnInvoke&, wpp::Env&, wpp::FnEnv*);
		std::string eval_intrinsic_file(wpp::node_t, const FnInvoke&, wpp::Env&, wpp::FnEnv*);
		std::string eval_intrinsic_use(wpp::node_t, const FnInvoke&, wpp::Env&, wpp::FnEnv*);

		std::string eval_fninvoke(wpp::node_t, const FnInvoke&, wpp::Env&, wpp::FnEnv*);
		std::string eval_fn(wpp::node_t, const Fn&, wpp::Env&, wpp::FnEnv*);
		std::string eval_codeify(wpp::node_t, const Codeify&, wpp::Env&, wpp::FnEnv*);
		std::string eval_varref(wpp::node_t, const VarRef&, wpp::Env&, wpp::FnEnv*);
		std::string eval_var(wpp::node_t, const Var&, wpp::Env&, wpp::FnEnv*);
		std::string eval_pop(wpp::node_t, const Pop&, wpp::Env&, wpp::FnEnv*);
		std::string eval_drop(wpp::node_t, const Drop&, wpp::Env&, wpp::FnEnv*);
		std::string eval_string(wpp::node_t, const String&, wpp::Env&, wpp::FnEnv*);
		std::string eval_new(wpp::node_t, const String&, wpp::Env&, wpp::FnEnv*);
		std::string eval_cat(wpp::node_t, const Concat&, wpp::Env&, wpp::FnEnv*);
		std::string eval_slice(wpp::node_t, const Concat&, wpp::Env&, wpp::FnEnv*);
		std::string eval_block(wpp::node_t, const Block&, wpp::Env&, wpp::FnEnv*);
		std::string eval_match(wpp::node_t, const Match&, wpp::Env&, wpp::FnEnv*);
		std::string eval_document(wpp::node_t, const Document&, wpp::Env&, wpp::FnEnv*);
	}
}


// Utils
namespace wpp { namespace {
	wpp::Fn find_func(
		wpp::node_t node_id,
		const View& name,
		size_t n_args,
		wpp::Env& env
	) {
		DBG();

		auto& functions = env.functions;
		const auto& ast = env.ast;
		const auto& flags = env.flags;

		// Lookup function which accepts at least n_args.
		if (auto it = functions.find(name); it != functions.end()) {
			auto& arities = it->second;

			if (auto arity_it = arities.lower_bound(n_args); arity_it != arities.end()) {
				auto& [min_args, entry] = *arity_it;

				if (flags & wpp::WARN_EXTRA_ARGS and n_args > min_args and not wpp::is_previously_seen_warning(WARN_EXTRA_ARGS, node_id, env))
					wpp::warning(report_modes::semantic, node_id, env, "extra arguments",
						wpp::cat("got ", n_args - min_args, " extra arguments (function expects >= ", min_args, " arguments)"),
						"this may be intentional behaviour, extra arguments will be pushed to the stack"
					);

				return ast.get<wpp::Fn>(entry.back());
			}
		}

		// No function found.
		wpp::error(report_modes::semantic, node_id, env, "function not found",
			wpp::cat("attempting to invoke function '", name, "' (", n_args, " parameters) which is undefined"),
			"are you passing the correct number of arguments?"
		);
	}


	std::string call_func(
		wpp::node_t node_id,
		const View& name,
		std::vector<std::string>& arg_strings,
		wpp::Env& env,
		wpp::FnEnv* fn_env
	) {
		DBG();

		auto& functions = env.functions;
		const auto& ast = env.ast;
		const auto& flags = env.flags;

		wpp::Fn func = wpp::find_func(node_id, name, arg_strings.size(), env);


		// Set up Arguments to pass down to function body.
		wpp::FnEnv new_fn_env;

		new_fn_env.arguments.emplace_back();

		if (fn_env)
			new_fn_env.arguments.back() = fn_env->arguments.back();



		const auto& params = func.parameters;

		// Handle variadic arguments.
		auto it = arg_strings.begin();

		for (; it != arg_strings.end() - params.size(); ++it)
			env.stack.back().emplace_back(*it);


		// Setup normal arguments.
		for (auto rit = params.rbegin(); rit != params.rend() and it != arg_strings.end(); ++rit, ++it) {
			const auto arg_it = new_fn_env.arguments.back().find(*rit);

			// If parameter is not already in environment, insert it.
			if (arg_it == new_fn_env.arguments.back().end())
				new_fn_env.arguments.back().emplace(*rit, *it);

			// If parameter exists, overwrite it.
			else {
				arg_it->second = *it;

				if (flags & wpp::WARN_PARAM_SHADOW_PARAM and not wpp::is_previously_seen_warning(WARN_PARAM_SHADOW_PARAM, node_id, env))
					wpp::warning(report_modes::semantic, node_id, env, "parameter shadows parameter",
						wpp::cat("parameter '", arg_it->first, "' inside function '", name, "' shadows parameter from enclosing function")
					);
			}
		}


		// Call function.
		env.call_depth++;

		if (flags & wpp::WARN_DEEP_RECURSION and env.call_depth >= 256 and not wpp::is_previously_seen_warning(WARN_DEEP_RECURSION, node_id, env))
			wpp::warning(report_modes::semantic, node_id, env, "deep recursion", wpp::cat("the call stack has grown to a depth of >= 256"),
				"this may indicate recursion without an exit condition"
			);

		std::string str = evaluate(func.body, env, &new_fn_env);

		env.call_depth--;

		new_fn_env.arguments.pop_back();

		return str;
	}
}}


namespace wpp { namespace {
	std::string eval_intrinsic_use(wpp::node_t node_id, const IntrinsicUse& use, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		return intrinsic_use(node_id, use.expr, env, fn_env);
	}

	std::string eval_intrinsic_file(wpp::node_t node_id, const IntrinsicFile& file, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		return intrinsic_file(node_id, file.expr, env, fn_env);
	}

	std::string eval_intrinsic_run(wpp::node_t node_id, const IntrinsicRun& run, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		return intrinsic_run(node_id, run.expr, env, fn_env);
	}

	std::string eval_intrinsic_pipe(wpp::node_t node_id, const IntrinsicPipe& pipe, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		return intrinsic_pipe(node_id, pipe.cmd, pipe.value, env, fn_env);
	}

	std::string eval_intrinsic_assert(wpp::node_t node_id, const IntrinsicAssert& ass, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		return intrinsic_assert(node_id, ass.lhs, ass.rhs, env, fn_env);
	}

	std::string eval_intrinsic_error(wpp::node_t node_id, const IntrinsicError& err, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		return intrinsic_error(node_id, err.expr, env, fn_env);
	}

	std::string eval_intrinsic_log(wpp::node_t node_id, const IntrinsicLog& log, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		return intrinsic_log(node_id, log.expr, env, fn_env);
	}


	std::string eval_fninvoke(wpp::node_t node_id, const FnInvoke& call, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		// Evaluate arguments.
		std::vector<std::string> arg_strings;
		const auto& args = call.arguments;

		for (auto it = args.rbegin(); it != args.rend(); ++it)
			arg_strings.emplace_back(wpp::evaluate(*it, env, fn_env));

		return wpp::call_func(node_id, call.identifier, arg_strings, env, nullptr);
	}


	std::string eval_fn(wpp::node_t node_id, const Fn& func, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		auto& functions = env.functions;
		const auto& flags = env.flags;

		const auto& name = func.identifier;
		const auto& params = func.parameters;
		const auto n_params = params.size();


		// Check if function already exists.
		if (auto it = functions.find(name); it != functions.end()) {
			auto& arities = it->second;

			if (auto arity_it = arities.find(n_params); arity_it != arities.end()) {
				auto& generations = arity_it->second;

				if (flags & wpp::WARN_FUNC_REDEFINED and not wpp::is_previously_seen_warning(WARN_FUNC_REDEFINED, node_id, env))
					wpp::warning(report_modes::semantic, node_id, env, "function redefined",
						wpp::cat("function '", name, "' (>=", n_params, " parameters) redefined")
					);

				generations.emplace_back(node_id);
			}

			else
				arities.emplace(n_params, std::initializer_list<node_t>{node_id});
		}

		// Otherwise, create it.
		else
			functions.emplace(name, std::map<size_t, std::vector<node_t>, std::greater<size_t>>{
				{n_params, std::initializer_list<node_t>{node_id}}
			});

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
		if (fn_env) {
			if (const auto it = fn_env->arguments.back().find(name); it != fn_env->arguments.back().end()) {
				// Check if it's shadowing a variable.
				if (
					flags & wpp::WARN_PARAM_SHADOW_VAR and
					not wpp::is_previously_seen_warning(WARN_PARAM_SHADOW_VAR, node_id, env) and
					variables.find(name) != variables.end()
				)
					wpp::warning(report_modes::semantic, node_id, env, "parameter shadows variable", wpp::cat("parameter '", name.str(), "' is shadowing a variable"));

				return it->second; // Return str.
			}
		}

		// Check if variable.
		if (const auto it = variables.find(name); it != variables.end())
			return it->second;

		wpp::error(report_modes::semantic, node_id, env, "variable not found",
			wpp::cat("attempting to reference variable '", name.str(), "' which is undefined")
		);

		return "";
	}


	std::string eval_var(wpp::node_t node_id, const Var& var, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		const auto& flags = env.flags;
		auto& variables = env.variables;

		const auto name = var.identifier;


		if (auto it = variables.find(name); it != variables.end()) {
			if (flags & wpp::WARN_VAR_REDEFINED and not wpp::is_previously_seen_warning(WARN_VAR_REDEFINED, node_id, env))
				wpp::warning(report_modes::semantic, node_id, env, "variable redefined", wpp::cat("variable '", name, "' redefined"));

			it->second = wpp::evaluate(var.body, env, fn_env);
		}

		else
			variables.emplace(name, wpp::evaluate(var.body, env, fn_env));

		return "";
	}


	std::string eval_pop(wpp::node_t node_id, const Pop& pop, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		auto& stack = env.stack;

		const auto& func = pop.identifier;
		const auto& args = pop.arguments;
		auto n_popped_args = pop.n_popped_args;



		// Evaluate arguments.
		std::vector<std::string> arg_strings;

		for (auto it = args.begin(); it != args.end(); ++it)
			arg_strings.emplace_back(wpp::evaluate(*it, env, fn_env));

		// Loop to collect as many strings from the stack as possible until we reach `n_popped_args`
		// or the stack is empty.
		while (n_popped_args--) {
			if (stack.back().empty())
				break;

			arg_strings.emplace_back(stack.back().back());
			stack.back().pop_back();
		}

		std::reverse(arg_strings.begin(), arg_strings.end());

		return wpp::call_func(node_id, func, arg_strings, env, nullptr);
	}


	std::string eval_new(wpp::node_t node_id, const New& nnew, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		env.stack.emplace_back();
		const std::string str = wpp::evaluate(nnew.expr, env, fn_env);
		env.stack.pop_back();

		return str;
	}


	std::string eval_drop(wpp::node_t node_id, const Drop& drop, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		auto& functions = env.functions;

		const auto& name = drop.identifier;
		const auto n_args = drop.n_args;

		if (auto it = functions.find(name); it != functions.end()) {
			auto& arities = it->second;

			if (auto arity_it = arities.find(n_args); arity_it != arities.end()) {
				// If we have found a function, drop the latest
				// generation and return to a previous definition.
				if (not arity_it->second.empty())
					arity_it->second.pop_back();

				// If there are no generations, erase the entry.
				if (arity_it->second.empty())
					arities.erase(arity_it);

				return "";
			}

			// If no functions exist under this name, remove the entire entry.
			if (arities.empty())
				functions.erase(it);
		}

		wpp::error(report_modes::semantic, node_id, env, "undefined function",
			wpp::cat("cannot drop undefined function '", name, "' (", n_args, " parameters)"),
			"are you passing the correct number of arguments?"
		);
	}


	std::string eval_string(wpp::node_t node_id, const String& str, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		return str.value;
	}


	std::string eval_cat(wpp::node_t node_id, const Concat& cat, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		std::string str;

		str += evaluate(cat.lhs, env, fn_env);
		str += evaluate(cat.rhs, env, fn_env);

		return str;
	}


	std::string eval_slice(wpp::node_t node_id, const Slice& s, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		std::string str = evaluate(s.expr, env, fn_env);

		int start = 0;
		int stop = 0;


		if (s.set & Slice::SLICE_STOP)
			stop = wpp::view_to_int(s.stop);

		if (s.set & Slice::SLICE_START or s.set & Slice::SLICE_INDEX)
			start = wpp::view_to_int(s.start);


		if (start < 0)
			start = str.size() + start;

		if (stop < 0)
			stop = str.size() + stop;


		// Just get character at index.
		if (s.set & Slice::SLICE_INDEX) {
			const char* const begin = str.data();
			const char* const end = str.data() + str.size();

			int i = 0;
			auto ptr = begin;

			// We need to loop here because we're dealing with UTF-8.
			for (; ptr != end and i != start; ptr += size_utf8(ptr))
				++i;

			// Return the character;
			return str.substr(i, size_utf8(ptr));
		}

		// If we have a stop index, remove chars from the end of the string.
		if (s.set & Slice::SLICE_STOP) {
			const char* const begin = str.data();
			const char* const end = str.data() + str.size();

			int erase_from_back = 0;

			// Translate stop index into UTF-8 index.
			for (auto ptr = begin; ptr != end and erase_from_back != stop; ptr += size_utf8(ptr))
				++erase_from_back;

			str.erase(erase_from_back, std::string::npos);
		}

		// If we have a start index, remove chars from the beginning of the string.
		if (s.set & Slice::SLICE_START) {
			const char* const begin = str.data();
			const char* const end = str.data() + str.size();

			int erase_from_front = 0;

			// Translate start index into UTF-8 index.
			for (auto ptr = begin; ptr != end and erase_from_front != start; ptr += size_utf8(ptr))
				++erase_from_front;

			str.erase(0, erase_from_front);
		}


		return str;
	}


	std::string eval_block(wpp::node_t node_id, const Block& block, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		for (const wpp::node_t node: block.statements)
			evaluate(node, env, fn_env);

		return evaluate(block.expr, env, fn_env);
	}


	std::string eval_match(wpp::node_t node_id, const Match& match, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();
		std::string str;

		const auto& test = match.expr;
		const auto& cases = match.cases;
		const auto& default_case = match.default_case;

		const auto test_str = evaluate(test, env, fn_env);

		// Compare test_str with arms of the match.
		auto it = std::find_if(cases.begin(), cases.end(), [&] (const auto& elem) {
			return test_str == evaluate(elem.first, env, fn_env);
		});

		// If found, evaluate the hand.
		if (it != cases.end())
			str = evaluate(it->second, env, fn_env);

		// If not found, check for a default arm, otherwise error.
		else {
			if (default_case == wpp::NODE_EMPTY)
				wpp::error(report_modes::semantic, node_id, env, "no matches found",
					"exhausted all checks in match expression"
				);

			else
				str = evaluate(default_case, env, fn_env);
		}

		return str;
	}


	std::string eval_document(wpp::node_t node_id, const Document& doc, wpp::Env& env, wpp::FnEnv* fn_env) {
		DBG();

		std::string str;

		for (const wpp::node_t node: doc.statements)
			str += evaluate(node, env, fn_env);

		return str;
	}
}}


namespace wpp {
	// The core of the evaluator.
	std::string evaluate(const wpp::node_t node_id, wpp::Env& env, wpp::FnEnv* fn_env) {
		try {
			return wpp::visit(env.ast[node_id],
				[&] (const IntrinsicRun& x)    { return eval_intrinsic_run    (node_id, x, env, fn_env); },
				[&] (const IntrinsicPipe& x)   { return eval_intrinsic_pipe   (node_id, x, env, fn_env); },
				[&] (const IntrinsicError& x)  { return eval_intrinsic_error  (node_id, x, env, fn_env); },
				[&] (const IntrinsicLog& x)    { return eval_intrinsic_log    (node_id, x, env, fn_env); },
				[&] (const IntrinsicAssert& x) { return eval_intrinsic_assert (node_id, x, env, fn_env); },
				[&] (const IntrinsicFile& x)   { return eval_intrinsic_file   (node_id, x, env, fn_env); },
				[&] (const IntrinsicUse& x)    { return eval_intrinsic_use    (node_id, x, env, fn_env); },

				[&] (const FnInvoke& x) { return eval_fninvoke (node_id, x, env, fn_env); },
				[&] (const Fn& x)       { return eval_fn       (node_id, x, env, fn_env); },
				[&] (const Codeify& x)  { return eval_codeify  (node_id, x, env, fn_env); },
				[&] (const VarRef& x)   { return eval_varref   (node_id, x, env, fn_env); },
				[&] (const Var& x)      { return eval_var      (node_id, x, env, fn_env); },
				[&] (const Pop& x)      { return eval_pop      (node_id, x, env, fn_env); },
				[&] (const New& x)      { return eval_new      (node_id, x, env, fn_env); },
				[&] (const Drop& x)     { return eval_drop     (node_id, x, env, fn_env); },
				[&] (const String& x)   { return eval_string   (node_id, x, env, fn_env); },
				[&] (const Concat& x)   { return eval_cat      (node_id, x, env, fn_env); },
				[&] (const Slice& x)    { return eval_slice    (node_id, x, env, fn_env); },
				[&] (const Block& x)    { return eval_block    (node_id, x, env, fn_env); },
				[&] (const Match& x)    { return eval_match    (node_id, x, env, fn_env); },
				[&] (const Document& x) { return eval_document (node_id, x, env, fn_env); }
			);
		}

		catch (const wpp::Report& e) {
			env.state |=
				wpp::ABORT_EVALUATION &
				wpp::ERROR_MODE_EVAL;

			throw;
		}
	}
}

