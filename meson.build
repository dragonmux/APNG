project(
	'APNG',
	'cpp',
	default_options: [
		'cpp_std=c++11',
		'warning_level=3',
		'buildtype=release'
	],
	version: '0.0.2',
	meson_version: '>= 0.48'
)

cxx = meson.get_compiler('cpp')

add_project_arguments(
	cxx.get_supported_arguments(
		'-Wvla', '-Wimplicit-fallthrough'
	), language: 'cpp'
)

if cxx.get_id() == 'msvc'
	add_project_arguments(
		'-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES=1',
		'-D_CRT_NONSTDC_NO_WARNINGS=1',
		language: 'cpp'
	)
endif

zlib = dependency('zlib')

APNGSrcs = [
	'crc32.cxx', 'stream.cxx', 'conversions.cxx', 'reader.cxx'
]

libAPNG = shared_library(
	'APNG',
	APNGSrcs,
	dependencies: [zlib],
	gnu_symbol_visibility: 'inlineshidden',
	install_rpath: '$ORIGIN',
	install: true,
	version: meson.project_version()
)
