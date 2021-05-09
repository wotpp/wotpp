#pragma once

#ifndef WOTPP_CONSTANTS
#define WOTPP_CONSTANTS

namespace wpp {
	constexpr auto MAX_EXPR_DEPTH = 256;  // Depth at which to warn about deeply nested expressions/statements
	constexpr auto MAX_REC_DEPTH  = 256;  // Depth at which to warn about deeply nested function calls
}

#endif
