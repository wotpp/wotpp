#pragma once

#ifndef WOTPP_FLAGS
#define WOTPP_FLAGS

#include <cstdint>
#include <misc/fwddecl.hpp>

namespace wpp {
	enum: flags_t {
		WARN_PARAM_SHADOW_FUNC  = 0b0000'0000'0001,
		WARN_PARAM_SHADOW_PARAM = 0b0000'0000'0010,
		WARN_FUNC_REDEFINED     = 0b0000'0000'0100,
		WARN_VARFUNC_REDEFINED  = 0b0000'0000'1000,
		WARN_ALL                = 0b0000'0000'1111,
		FLAG_DISABLE_RUN        = 0b0001'0000'0000,
		INTERNAL_ERROR_STATE    = 0b1000'0000'0000,
	};
}

#endif
