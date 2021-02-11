# https://mesonbuild.com/Builtin-options.html

project(
	'wot++',
	'cpp',
	license: 'MPL-2.0',
	version: '1.0',
	default_options: ['cpp_std=c++17', 'buildtype=release', 'strip=true', 'warning_level=2']
)

extra_opts = []

modules_inc = include_directories('modules/')

sources = files('src/main.cpp')
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


# Sanitizer support.
if get_option('sanitizers')
	if meson.get_compiler('cpp').has_argument('-fsanitize=address,undefined')
		extra_opts += ['b_sanitize=address,undefined']

	else
		error('sanitizers are not unsupported')
	endif
endif


if get_option('disable_run')
	add_project_arguments('-DWPP_DISABLE_RUN', language: 'cpp')
endif

libasan_dep = meson.get_compiler('cpp').find_library('asan', required: get_option('sanitizers'))
libstdcppfs_dep = meson.get_compiler('cpp').find_library('stdc++fs', required: false)
libreadline_dep = dependency('readline', required: false)

if get_option('disable_repl') or not libreadline_dep.found()
	add_project_arguments('-DWPP_DISABLE_REPL', language: 'cpp')
endif


exe = executable(
	'w++',
	sources,
	include_directories: [sources_inc, modules_inc],
	dependencies: [libstdcppfs_dep, libasan_dep, libreadline_dep],
	install: true,
	override_options: extra_opts
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
	'tests/prefix.wpp': true,
	'tests/error_no_func.wpp': false,
	'tests/assert_fail.wpp': false,
	'tests/run_fail.wpp': false,
	'tests/error.wpp': false,
	'tests/conflict_keyword.wpp': false,
}

if not get_option('disable_run')
    test_cases += {'tests/run.wpp': true}
endif

foreach case, should_pass: test_cases
	test(case, test_runner, args: [files(case), exe], should_fail: not should_pass)
endforeach


