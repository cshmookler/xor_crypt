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
    input : src_dir / 'version.hpp.in',
    output : 'version.hpp',
)

executable(
    'xorc',
    files(src_dir / 'version.cpp', src_dir / 'main.cpp'),
    dependencies : project_deps,
    install : true,
)

tests = [
    [ 'version', files(src_dir / 'version.test.cpp'), project_deps ],
    # Add additional tests here.
]

foreach test : tests
    test_exec = executable(
        test[0], # name
        test[1], # source files
        dependencies : test[2], # dependencies
    )
    test(test[0], test_exec)
endforeach

