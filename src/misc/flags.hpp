#pragma once

#ifndef WOTPP_FLAGS
#define WOTPP_FLAGS

#include <cstdint>
#include <misc/fwddecl.hpp>

namespace wpp {
	enum: flags_t {
		WARN_PARAM_SHADOW_VAR   = 0b000000000000000001,
		WARN_PARAM_SHADOW_PARAM = 0b000000000000000010,
		WARN_FUNC_REDEFINED     = 0b000000000000000100,
		WARN_VAR_REDEFINED      = 0b000000000000001000,
		WARN_DEEP_RECURSION     = 0b000000000000010000,
		WARN_DEEP_EXPRESSION    = 0b000000000000100000,
		WARN_EXTRA_ARGS         = 0b000000000001000000,

		WARN_ALL                = 0b000000000001111111,
		WARN_USEFUL             = 0b000000000000000111,

		FLAG_INLINE_REPORTS     = 0b000000000010000000,

		FLAG_DISABLE_RUN        = 0b000000000100000000,
		FLAG_DISABLE_FILE       = 0b000000001000000000,
		FLAG_DISABLE_COLOUR     = 0b000000010000000000,

		ERROR_MODE_PARSE        = 0b000000100000000000,
		ERROR_MODE_LEX          = 0b000001000000000000,
		ERROR_MODE_UTF8         = 0b000010000000000000,
		ERROR_MODE_EVAL         = 0b000100000000000000,

		ABORT_ERROR_RECOVERY    = 0b001000000000000000,
		ABORT_EVALUATION        = 0b010000000000000000,

		FLAG_DEFAULT            = WARN_DEEP_EXPRESSION | WARN_DEEP_RECURSION,
	};
}

#endif
