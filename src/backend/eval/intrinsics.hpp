#pragma once

#ifndef WOTPP_INTRINSICS
#define WOTPP_INTRINSICS

#include <string>
#include <vector>

#include <structures/environment.hpp>

namespace wpp {
	std::string intrinsic_assert (const wpp::node_t, const std::vector<wpp::node_t>&, wpp::Env&, wpp::FnEnv* = nullptr);
	std::string intrinsic_slice  (const wpp::node_t, const std::vector<wpp::node_t>&, wpp::Env&, wpp::FnEnv* = nullptr);
	std::string intrinsic_find   (const wpp::node_t, const std::vector<wpp::node_t>&, wpp::Env&, wpp::FnEnv* = nullptr);
	std::string intrinsic_error  (const wpp::node_t, const std::vector<wpp::node_t>&, wpp::Env&, wpp::FnEnv* = nullptr);
	std::string intrinsic_file   (const wpp::node_t, const std::vector<wpp::node_t>&, wpp::Env&, wpp::FnEnv* = nullptr);
	std::string intrinsic_source (const wpp::node_t, const std::vector<wpp::node_t>&, wpp::Env&, wpp::FnEnv* = nullptr);
	std::string intrinsic_log    (const wpp::node_t, const std::vector<wpp::node_t>&, wpp::Env&, wpp::FnEnv* = nullptr);
	std::string intrinsic_escape (const wpp::node_t, const std::vector<wpp::node_t>&, wpp::Env&, wpp::FnEnv* = nullptr);
	std::string intrinsic_length (const wpp::node_t, const std::vector<wpp::node_t>&, wpp::Env&, wpp::FnEnv* = nullptr);
	std::string intrinsic_eval   (const wpp::node_t, const std::vector<wpp::node_t>&, wpp::Env&, wpp::FnEnv* = nullptr);
	std::string intrinsic_run    (const wpp::node_t, const std::vector<wpp::node_t>&, wpp::Env&, wpp::FnEnv* = nullptr);
	std::string intrinsic_pipe   (const wpp::node_t, const std::vector<wpp::node_t>&, wpp::Env&, wpp::FnEnv* = nullptr);
}

#endif


