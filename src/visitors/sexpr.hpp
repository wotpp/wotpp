// #pragma once

// #ifndef WOTPP_SEXPR
// #define WOTPP_SEXPR

// #include <string>

// #include <utils/util.hpp>
// #include <structures/ast.hpp>
// #include <parser.hpp>

// // AST visitor that prints an s-expression.

// namespace wpp {
// 	template <typename T>
// 	inline void print_ast(const T& variant, const wpp::AST& tree, std::string& str) {
// 		wpp::visit(variant,
// 			[&] (const FnInvoke& call) {
// 				const auto& [name, args, pos] = call;

// 				str += "( '" + name + "(...)' ";

// 				for (const wpp::node_t x: args) {
// 					print_ast(tree[x], tree, str);
// 					str += " ";
// 				}

// 				str += ")";
// 			},

// 			[&] (const FnRun& run) {
// 				const auto& [arg, pos] = run;
// 				str += "( 'run(...)' ";
// 					print_ast(tree[arg], tree, str);
// 				str += " )";
// 			},

// 			[&] (const FnAssert& ass) {
// 				const auto& [args, pos] = ass;
// 				str += "( 'assert(...)' ";
// 					print_ast(tree[args.first], tree, str);
// 				str += " ";
// 					print_ast(tree[args.second], tree, str);
// 				str += " )";
// 			},

// 			[&] (const FnFile& file) {
// 				const auto& [arg, pos] = file;
// 				str += "( 'file(...)' ";
// 					print_ast(tree[arg], tree, str);
// 				str += " )";
// 			},

// 			[&] (const FnEval& eval) {
// 				const auto& [arg, pos] = eval;
// 				str += "( 'eval(...)' ";
// 					print_ast(tree[arg], tree, str);
// 				str += " )";
// 			},

// 			[&] (const FnPipe& pipe) {
// 				const auto& [arg, pos] = pipe;
// 				str += "( 'pipe(...)' ";
// 					print_ast(tree[arg], tree, str);
// 				str += " )";
// 			},

// 			[&] (const FnError& err) {
// 				const auto& [arg, pos] = err;
// 				str += "( 'error(...)' ";
// 					print_ast(tree[arg], tree, str);
// 				str += " )";
// 			},

// 			[&] (const Fn& func) {
// 				const auto& [name, params, body, pos] = func;

// 				str += "( 'fn " + name + "(";

// 				if (not params.empty()) {
// 					for (auto it = params.begin(); it != params.end() - 1; ++it)
// 						str += *it + ", ";

// 					str += params.back();
// 				}

// 				str += ")' ";

// 				print_ast(tree[body], tree, str);

// 				str += " )";
// 			},

// 			[&] (const String& x) {
// 				str += "'\"" + x.value + "\"'";
// 			},

// 			[&] (const Concat& cat) {
// 				const auto& [lhs, rhs, pos] = cat;

// 				str += "( .. ";
// 					print_ast(tree[lhs], tree, str);
// 				str += " ";
// 					print_ast(tree[rhs], tree, str);
// 				str += " )";
// 			},

// 			[&] (const Block& blck) {
// 				const auto& [stmts, expr, pos] = blck;

// 				str += "( '{...}' ";

// 				for (const wpp::node_t s: stmts) {
// 					print_ast(tree[s], tree, str);
// 					str += " ";
// 				}

// 				print_ast(tree[expr], tree, str);

// 				str += " )";
// 			},

// 			[&] (const Map& map) {
// 				const auto& [match_against, cases, default_case] = map;

// 				// str += "";
// 			},

// 			[&] (const Pre& pre) {
// 				const auto& [name, stmts, pos] = pre;

// 				str += "( 'namespace \\\"" + name + "\\\"' ";

// 				for (const wpp::node_t s: stmts) {
// 					print_ast(tree[s], tree, str);
// 					str += " ";
// 				}

// 				str += " )";
// 			},

// 			[&] (const Document& x) {
// 				for (const wpp::node_t s: x.stmts) {
// 					print_ast(tree[s], tree, str);
// 					str += " ";
// 				}
// 			}
// 		);
// 	}


// 	inline std::string print_ast(wpp::node_t root, const wpp::AST& tree) {
// 		std::string str;
// 		str.reserve(1024 * 1024);
// 		wpp::print_ast(tree[root], tree, str);
// 		return str;
// 	}
// }

// #endif

