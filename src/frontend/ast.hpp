#pragma once

#ifndef WOTPP_AST
#define WOTPP_AST

#include <vector>
#include <variant>
#include <utility>

#include <misc/fwddecl.hpp>
#include <misc/dbg.hpp>

// A vector of variants.

namespace wpp {
	constexpr node_t NODE_EMPTY = -1;

	template <typename... Ts>
	class HeterogenousVector: public std::vector<std::variant<Ts...>> {
		using std::vector<std::variant<Ts...>>::vector;

		public:
			// Construct element in place and return its index.
			template <typename T, typename... Xs>
			node_t add(Xs&&... args) {
				DBG();
				this->emplace_back(std::in_place_type<T>, std::forward<Xs>(args)...);
				return static_cast<int64_t>(this->size() - 1);
			}

			template <typename T, typename... Xs>
			auto& replace(node_t i, Xs&&... args) {
				DBG();
				return (*this)[i].template emplace<T>(std::forward<Xs>(args)...);
			}

			// Retrieve element by index and pull the underlying type out of
			// the variant.
			template <typename T>
			T& get(node_t i) {
				DBG();
				return std::get<T>((*this)[i]);
			}

			template <typename T>
			const T& get(node_t i) const {
				DBG();
				return std::get<T>((*this)[i]);
			}
	};
}

#endif
