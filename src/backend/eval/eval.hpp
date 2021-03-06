#pragma once

#ifndef WOTPP_EVAL
#define WOTPP_EVAL

#include <string>

#include <structures/environment.hpp>

namespace wpp {
	std::string evaluate(const wpp::node_t, wpp::Env&, wpp::FnEnv* = nullptr);
}

#endif

