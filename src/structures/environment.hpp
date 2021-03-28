#pragma once

#ifndef WOTPP_ENV
#define WOTPP_ENV

#include <filesystem>
#include <string>
#include <vector>
#include <stack>
#include <list>
#include <unordered_map>

#include <cstdint>

#include <misc/flags.hpp>
#include <misc/fwddecl.hpp>
#include <frontend/parser/ast_nodes.hpp>


namespace wpp {
	struct FuncKey {
		wpp::View identifier{};
		int n_args{};

		bool operator==(const FuncKey& other) const {
			return n_args == other.n_args and identifier == other.identifier;
		}
	};
}


namespace std {
	template<> struct hash<wpp::FuncKey> {
		std::size_t operator()(const wpp::FuncKey& v) const noexcept {
			return std::hash<wpp::View>()(v.identifier) ^ std::hash<int>()(v.n_args);
		}
	};
}


namespace wpp {
	struct Source {
		const std::filesystem::path file{};
		const char* const base = nullptr;
		const wpp::mode_type_t mode{};

		Source(
			const std::filesystem::path& file_,
			const char* const base_,
			const wpp::mode_type_t mode_
		):
			file(file_),
			base(base_),
			mode(mode_) {}
	};


	struct Pos {
		const wpp::Source& source;           // Source associated with this position.
		wpp::View view;
	};


	struct VariadicFuncEntry {
		std::vector<wpp::node_t> generations{};
		int min_args{};
	};


	using Variables         = std::unordered_map<wpp::View, std::vector<std::string>>;
	using VariadicFunctions = std::unordered_map<wpp::View, wpp::VariadicFuncEntry>;
	using Functions         = std::unordered_map<wpp::FuncKey, std::vector<wpp::node_t>>;

	using Arguments = std::unordered_map<wpp::View, std::string>;
	using Positions = std::vector<wpp::Pos>;


	struct FnEnv {
		wpp::Arguments arguments;
		wpp::Arguments priority_constants;
	};


	struct Sources {
		std::list<wpp::Source> sources{};
		std::list<std::string> strings{};

		wpp::Source& push(const std::filesystem::path& file, const std::string& str, const wpp::mode_type_t mode) {
			const auto& ref = strings.emplace_back(str);
			return sources.emplace_back(file, ref.c_str(), mode);
		}

		void pop() {
			sources.pop_back();
			strings.pop_back();
		}

		const wpp::Source& top() const {
			return sources.back();
		}
	};


	struct Env {
		wpp::AST ast{};

		wpp::VariadicFunctions variadic_functions{};
		wpp::Functions functions{};
		wpp::Variables variables{};

		std::stack<std::string, std::vector<std::string>> stack{};

		wpp::Positions positions{};
		wpp::Sources sources{};

		const std::filesystem::path root{};
		const wpp::flags_t flags{};
		wpp::flags_t state{};

		int call_depth{};


		Env(
			const std::filesystem::path& root_,
			const wpp::flags_t flags_
		):
			root(root_),
			flags(flags_)
		{
			ast.reserve(ast.capacity() + (1024 * 1024 * 10) / sizeof(decltype(ast)::value_type));
		}
	};
}

#endif
