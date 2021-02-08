project('wot++', 'cpp',
	license: 'MPL-2.0',
	version: '1.0',
	default_options: ['cpp_std=c++17'])

modules_inc = include_directories('modules/')

sources = files('src/main.cpp')
sources_inc = include_directories('src/')

if meson.get_compiler('cpp').get_id() == 'clang'
	add_project_arguments('-ferror-limit=2', language: 'cpp')
elif meson.get_compiler('cpp').get_id() == 'gcc'
	add_project_arguments('-fmax-errors=2', language: 'cpp')
endif

if get_option('profile')
	if meson.get_compiler('cpp').has_argument('-finstrument-functions')
		add_project_arguments('-finstrument-functions', language: 'cpp')
	else
		warning('Profiling is unsupported')
	endif
endif

if get_option('disable_run')
    add_project_arguments('-DWPP_DISABLE_RUN', language: 'cpp')
endif

libstdcppfs_dep = meson.get_compiler('cpp').find_library('stdc++fs', required: false)

exe = executable('wotpp', sources,
	include_directories: [sources_inc, modules_inc],
	dependencies: [libstdcppfs_dep],
	install: true)

test_runner = find_program('tests/run_test.py')
test('md test', test_runner, args: [files('tests/md.wpp'), exe])
if not get_option('disable_run')
   test('run test', test_runner, args: [files('tests/run.wpp'), exe])
endif
test('eval test', test_runner, args: [files('tests/eval.wpp'), exe])
