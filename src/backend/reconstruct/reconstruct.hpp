// #pragma once

// #ifndef WOTPP_RECONSTRUCT
// #define WOTPP_RECONSTRUCT

// #include <string>

// #include <utils/util.hpp>
// #include <structures/ast.hpp>
// #include <parser.hpp>

// // AST visitor that prints a reconstruction of the source.

// namespace wpp {
// 	inline std::string reconstruct_source(const wpp::node_t node, const wpp::AST& tree) {
// 		auto& variant = tree[node];
// 		std::string str;

// 		wpp::visit(variant,
// 			[&] (const FnInvoke& call) {
// 				const auto& [name, args, pos] = call;

// 				str += name + "(";

// 				if (not args.empty()) {
// 					for (auto it = args.begin(); it != args.end() - 1; ++it)
// 						str += reconstruct_source(*it, tree) + ", ";

// 					str += reconstruct_source(args.back(), tree);
// 				}

// 				str += ")";
// 			},

// 			[&] (const Intrinsic& fn) {
// 				const auto& [type, name, exprs, pos] = fn;

// 				str += name + "(";

// 				if (not exprs.empty()) {
// 					for (auto it = exprs.begin(); it != exprs.end() - 1; ++it)
// 						str += reconstruct_source(*it, tree) + ", ";

// 					str += reconstruct_source(exprs.back(), tree);
// 				}

// 				str += ")";
// 			},

// 			[&] (const Fn& func) {
// 				const auto& [name, params, body, pos] = func;

// 				str += "let " + name + "(";

// 				if (not params.empty()) {
// 					for (auto it = params.begin(); it != params.end() - 1; ++it)
// 						str += *it + ", ";

// 					str += params.back();
// 				}

// 				str += ") " + reconstruct_source(body, tree);
// 			},

// 			[&] (const Var& var) {
// 				const auto& [name, body, pos] = var;

// 				str += "var " + name + " ";
// 				str += reconstruct_source(body, tree);
// 			},

// 			[&] (const Drop& drop) {
// 				const auto& [func, pos] = drop;

// 				str += "drop ";
// 				str += reconstruct_source(func, tree);
// 			},

// 			[&] (const String& x) {
// 				str = '"' + x.value + '"';
// 			},

// 			[&] (const Concat& cat) {
// 				const auto& [lhs, rhs, pos] = cat;
// 				str += reconstruct_source(lhs, tree) + " .. " + reconstruct_source(rhs, tree);
// 			},

// 			[&] (const Block& blck) {
// 				const auto& [stmts, expr, pos] = blck;

// 				str += "{ ";

// 				for (const wpp::node_t s: stmts)
// 					str += reconstruct_source(s, tree) + " ";

// 				str += reconstruct_source(expr, tree) + " }";
// 			},

// 			[&] (const Map& map) {
// 				const auto& [match_against, cases, default_case, pos] = map;

// 				str += "map " + reconstruct_source(match_against, tree) + " {\n";

// 				for (const auto& [lhs, rhs]: cases) {
// 					str += "\t" + reconstruct_source(lhs, tree) + " -> " + reconstruct_source(rhs, tree) + "\n";
// 				}

// 				str += "\t* -> " + reconstruct_source(default_case, tree) + "\n";
// 				str += "}";
// 			},

// 			[&] (const Pre& pre) {
// 				const auto& [exprs, stmts, pos] = pre;

// 				std::string name;

// 				for (auto it = exprs.rbegin(); it != exprs.rend(); ++it)
// 					name += reconstruct_source(*it, tree);

// 				str += "prefix " + name + " { ";

// 				for (const wpp::node_t s: stmts)
// 					str += reconstruct_source(s, tree) + " ";

// 				str += " }";
// 			},

// 			[&] (const Document& x) {
// 				for (const wpp::node_t s: x.stmts)
// 					str += reconstruct_source(s, tree) + " ";
// 			}
// 		);

// 		return str;
// 	}
// }

// #endif

