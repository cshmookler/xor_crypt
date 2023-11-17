project(
    meson.get_external_property('name', 'unknown'),
    'cpp',
    version : meson.get_external_property('version', 'unknown'),
    default_options : [ 'cpp_std=c++11' ],
)

project_name = meson.project_name()
if project_name == 'unknown'
    error('Failed to get project name from Conan.')
endif

project_version = meson.project_version()
if project_version == 'unknown'
    error('Failed to get project version from Conan.')
endif

project_dep_strings = meson.get_external_property('deps', [ 'unknown' ])
if project_dep_strings == [ 'unknown' ]
    error('Failed to get project dependencies from Conan.')
endif

project_deps = []
foreach dep : project_dep_strings
    project_deps += dependency(
        dep[0],
        modules : dep[1],
        version : dep[2],
        required : true,
        method : 'auto',
    )
endforeach

include_dir = meson.source_root() / 'include' / project_name
src_dir = meson.source_root() / 'src'
build_dir = meson.build_root()

conf_data = configuration_data()
conf_data.set('version', project_version)
configure_file(
    configuration : conf_data,
    input : 'src' / 'version.hpp.in',
    output : 'version.hpp',
)

headers = files(
    build_dir / 'version.hpp',
    # Add additional header files here.
)
sources = files(
    src_dir / 'version.cpp',
    # Add additional source files here.
)

sources += src_dir / 'main.cpp'
xor_crypt = executable(
    'xorc',
    sources,
    dependencies : project_deps,
    install : true,
)

