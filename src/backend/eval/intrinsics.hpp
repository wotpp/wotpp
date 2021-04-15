#pragma once

#ifndef WOTPP_INTRINSICS
#define WOTPP_INTRINSICS

#include <string>
#include <vector>

#include <structures/environment.hpp>

namespace wpp {
	std::string intrinsic_log    (wpp::node_t, wpp::node_t, wpp::Env&,              wpp::FnEnv* = nullptr);
	std::string intrinsic_error  (wpp::node_t, wpp::node_t, wpp::Env&,              wpp::FnEnv* = nullptr);
	std::string intrinsic_assert (wpp::node_t, wpp::node_t, wpp::node_t, wpp::Env&, wpp::FnEnv* = nullptr);
	std::string intrinsic_file   (wpp::node_t, wpp::node_t, wpp::Env&,              wpp::FnEnv* = nullptr);
	std::string intrinsic_use    (wpp::node_t, wpp::node_t, wpp::Env&,              wpp::FnEnv* = nullptr);
	std::string intrinsic_eval   (wpp::node_t, wpp::node_t, wpp::Env&,              wpp::FnEnv* = nullptr);
	std::string intrinsic_run    (wpp::node_t, wpp::node_t, wpp::Env&,              wpp::FnEnv* = nullptr);
	std::string intrinsic_pipe   (wpp::node_t, wpp::node_t, wpp::node_t, wpp::Env&, wpp::FnEnv* = nullptr);
}

#endif


