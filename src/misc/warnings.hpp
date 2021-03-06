#pragma once

#ifndef WOTPP_WARNINGS
#define WOTPP_WARNINGS

#include <cstdint>
#include <misc/fwddecl.hpp>

namespace wpp {
	enum: warning_t {
		WARN_PARAM_SHADOW_FUNC  = 0b00000001,
		WARN_PARAM_SHADOW_PARAM = 0b00000010,
		WARN_FUNC_REDEFINED     = 0b00000100,
		WARN_VARFUNC_REDEFINED  = 0b00001000,
		WARN_ALL                = 0b11111111,
	};
}

#endif
