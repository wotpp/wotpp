# https://mesonbuild.com/Builtin-options.html

project(
	'wot++',
	'cpp',
	license: 'MPL-2.0',
	version: '1.0',
	default_options: [
		'cpp_std=c++17',
		'buildtype=release',
		'native=true',
		'strip=true',
		'warning_level=2',
		'b_lto_threads=4',
		'b_lto=true',
		'b_ndebug=if-release',
	]
)

extra_cxx_opts = [
	'-pedantic',
	'-Wextra',
	'-Wnull-dereference',
	'-Wshadow',
	'-Wno-unused',
	'-Wdisabled-optimization',
	'-Wundef',
]

if meson.get_compiler('cpp').has_argument('-Wlogical-op')
	extra_cxx_opts += '-Wlogical-op'
endif

if meson.get_compiler('cpp').has_argument('-Wduplicated-branches')
	extra_cxx_opts += '-Wduplicated-branches'
endif

if meson.get_compiler('cpp').has_argument('-Wduplicated-cond')
	extra_cxx_opts += '-Wduplicated-cond'
endif

extra_opts = []
deps = []

sources = files(
	'src/main.cpp',

	'src/misc/fwddecl.hpp',
	'src/structures/environment.hpp',

	'src/misc/util/util.hpp',
	'src/misc/util/util.cpp',

	'src/misc/repl.hpp',
	'src/misc/flags.hpp',

	'src/frontend/ast.hpp',

	'src/frontend/lexer/lexer.hpp',
	'src/frontend/lexer/lexer.cpp',

	'src/frontend/parser/ast_nodes.hpp',
	'src/frontend/parser/parser.hpp',
	'src/frontend/parser/parser.cpp',

	'src/frontend/token.hpp',
	'src/frontend/view.hpp',

	'src/frontend/char.hpp',

	'src/backend/eval/intrinsics.hpp',
	'src/backend/eval/intrinsics.cpp',
	'src/backend/eval/eval.hpp',
	'src/backend/eval/eval.cpp',
)

sources_inc = include_directories('src/')


# Set error limit.
if meson.get_compiler('cpp').get_id() == 'clang'
	add_project_arguments('-ferror-limit=2', language: 'cpp')

elif meson.get_compiler('cpp').get_id() == 'gcc'
	add_project_arguments('-fmax-errors=2', language: 'cpp')
endif


# Profiling support.
if get_option('profile')
	if meson.get_compiler('cpp').has_argument('-finstrument-functions')
		add_project_arguments('-finstrument-functions', language: 'cpp')

	else
		error('profiling is unsupported')
	endif
endif


# Native build `-march=native`.
if get_option('profile')
	if meson.get_compiler('cpp').has_argument('-march=native')
		add_project_arguments('-march=native', language: 'cpp')

	else
		error('-march=native is unsupported')
	endif
endif


# Look for libstdc++fs when using GCC or clang (except clang that emulates msvc)
if meson.get_compiler('cpp').get_argument_syntax() == 'gcc'
	deps += meson.get_compiler('cpp').find_library('stdc++fs', required: false)
endif


# Sanitizer support.
if get_option('sanitizers')
	extra_opts += 'b_sanitize=address,undefined'
	deps += meson.get_compiler('cpp').find_library('asan', required: false)
endif

if get_option('disable_run')
	add_project_arguments('-DWPP_DISABLE_RUN', language: 'cpp')
endif


# REPL stuff
libreadline_dep = dependency('readline', required: false)

if get_option('disable_repl') or not libreadline_dep.found()
	add_project_arguments('-DWPP_DISABLE_REPL', language: 'cpp')
else
	deps += libreadline_dep
endif

exe = executable(
	'w++',
	sources,
	include_directories: [sources_inc],
	dependencies: deps,
	install: true,
	override_options: extra_opts,
	cpp_args: extra_cxx_opts
)

# Test cases
test_runner = find_program('tests/run_test.py')

# Flag indicates whether test is expected to fail or not.
# true = test should pass
# false = test should fail
test_cases = {
	'tests/func.wpp': true,
	'tests/eval.wpp': true,
	'tests/nested_func.wpp': true,
	'tests/comments.wpp': true,
	'tests/blocks.wpp': true,
	'tests/strings.wpp': true,
	'tests/assert.wpp': true,
	'tests/file.wpp': true,
	'tests/meta.wpp': true,
	'tests/literals.wpp': true,
	'tests/paragraph.wpp': true,
	'tests/raw.wpp': true,
	'tests/code.wpp': true,
	'tests/escape.wpp': true,
	'tests/map.wpp': true,
	'tests/error_no_func.wpp': false,
	'tests/assert_fail.wpp': false,
	'tests/error.wpp': false,
	'tests/conflict_keyword.wpp': false,
	'tests/slice.wpp': true,
	'tests/slice_fail.wpp': false,
	'tests/find.wpp': true,
	'tests/length.wpp': true,
	'tests/smart_strings.wpp': true,
	'tests/generational_func.wpp': true,
	'tests/source.wpp': true,
	'tests/stringify.wpp': true,
	'tests/codeify.wpp': true,
	'tests/var.wpp': true,
	'tests/drop.wpp': true,
	'tests/drop_fail.wpp': false,
	'tests/utf_invalid.wpp': false,
	'tests/utf_valid.wpp': true,
}

if not get_option('disable_run')
	test_cases += {'tests/run_fail.wpp': false}
	test_cases += {'tests/run.wpp': true}
	test_cases += {'tests/pipe.wpp': true}
endif

foreach case, should_pass: test_cases
	test(case, test_runner, args: [exe, files(case)], should_fail: not should_pass)
endforeach
