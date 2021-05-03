#pragma once

#ifndef WOTPP_FWDDECL
#define WOTPP_FWDDECL

#include <cstdint>

namespace wpp {
	struct Lexer;
	struct Env;
	struct Source;
	struct Pos;


	using flags_t = uint32_t;
	using node_t = int32_t;

	using token_type_t = uint8_t;

	using mode_type_t = uint8_t;
	using lexer_mode_type_t = uint8_t;
	using report_mode_type_t = uint8_t;
	using report_type_type_t = uint8_t;


	namespace modes {
		#define MODES \
			MODE(repl) \
			MODE(source) \
			MODE(eval) \
			MODE(normal)

		#define MODE(x) x,
			enum: mode_type_t { MODES };
		#undef MODE

		#define MODE(x) #x,
			constexpr const char* mode_to_str[] = { MODES };
		#undef MODE

		#undef MODES
	}


	namespace report_modes {
		#define MODES \
			MODE(eval) \
			MODE(lexer) \
			MODE(parser) \
			MODE(utf8)

		#define MODE(x) x,
			enum: report_mode_type_t { MODES };
		#undef MODE

		#define MODE(x) #x,
			constexpr const char* report_mode_to_str[] = { MODES };
		#undef MODE

		#undef MODES
	}


	namespace report_types {
		#define MODES \
			MODE(error) \
			MODE(warning) \
			MODE(utf8)

		#define MODE(x) x,
			enum: report_type_type_t { MODES };
		#undef MODE

		#define MODE(x) #x,
			constexpr const char* report_type_to_str[] = { MODES };
		#undef MODE

		#undef MODES
	}


	namespace lexer_modes {
		#define MODES \
			MODE(normal) \
			MODE(chr) \
			\
			MODE(slice) \
			MODE(args_or_params) \
			\
			MODE(string) \
			MODE(string_raw) \
			MODE(string_para) \
			MODE(string_code)

		#define MODE(x) x,
			enum: lexer_mode_type_t { MODES };
		#undef MODE

		#define MODE(x) #x,
			constexpr const char* lexer_mode_to_str[] = { MODES };
		#undef MODE

		#undef MODES
	}
}

#endif
