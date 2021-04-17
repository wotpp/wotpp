#pragma once

#ifndef WOTPP_ENV
#define WOTPP_ENV

#include <filesystem>
#include <string>
#include <vector>
#include <stack>
#include <list>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include <cstdint>

#include <misc/flags.hpp>
#include <misc/fwddecl.hpp>
#include <frontend/parser/ast_nodes.hpp>


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
		const wpp::Source& source;   // Source associated with this position.
		wpp::View view;
	};


	using Variables = std::unordered_map<wpp::View, std::string>;
	using Functions = std::unordered_map<wpp::View, std::map<size_t, std::vector<wpp::node_t>, std::greater<size_t>>>;

	using Arguments = std::unordered_map<wpp::View, std::string>;
	using Positions = std::vector<wpp::Pos>;


	using SearchPath = std::vector<std::filesystem::path>;


	struct FnEnv {
		wpp::Arguments arguments;
	};


	struct Sources {
		std::list<wpp::Source> sources{};
		std::list<std::string> strings{};
		std::unordered_set<std::string> previously_seen{};

		bool is_previously_seen(const std::filesystem::path& p) const {
			return previously_seen.find(p.string()) != previously_seen.end();
		}

		wpp::Source& push(const std::filesystem::path& file, const std::string& str, const wpp::mode_type_t mode) {
			previously_seen.emplace(file.string());
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

		wpp::Functions functions{};
		wpp::Variables variables{};

		std::vector<std::vector<std::string>> stack{};

		wpp::Positions positions{};
		wpp::Sources sources{};

		const std::filesystem::path root{};
		const wpp::SearchPath path{};

		const wpp::flags_t flags{};
		wpp::flags_t state{};

		size_t call_depth{};


		Env(
			const std::filesystem::path& root_,
			const wpp::SearchPath& path_,
			const wpp::flags_t flags_
		):
			root(root_),
			path(path_),
			flags(flags_)
		{
			ast.reserve(ast.capacity() + (1024 * 1024 * 10) / sizeof(decltype(ast)::value_type)); // 10MiB tree.
			stack.emplace_back(); // Root stack.
		}
	};
}

#endif
