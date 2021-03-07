#pragma once

#ifndef WOTPP_ENV
#define WOTPP_ENV

#include <filesystem>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>

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
		const wpp::Source& source;           // Source associated with this position.
		const char* const offset = nullptr;  // Offset into the source.
	};


	using Functions = std::unordered_map<std::string, std::vector<wpp::node_t>>;
	using Variables = std::unordered_map<std::string, std::vector<std::string>>;
	using Arguments = std::unordered_map<wpp::View, std::string>;
	using Positions = std::vector<wpp::Pos>;


	struct FnEnv {
		wpp::Arguments args;
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

		wpp::Functions functions{};
		wpp::Variables variables{};
		wpp::Positions positions{};

		const std::filesystem::path root{};
		const wpp::flags_t flags{};

		wpp::Sources sources{};

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
