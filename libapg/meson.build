# NurOS Ruzen42 2025
project('libapg', 'cpp',
        version: '0.1.0',
        default_options: [
            'cpp_std=c++17',
            'warning_level=3',
            'buildtype=debugoptimized',
            'default_library=both'
        ]
)

cpp = meson.get_compiler('cpp')

nlohmann_json_dep = dependency('nlohmann_json', required: true)
lmdb_dep = dependency('lmdb', required: true)
libarchive_dep = dependency('libarchive', required: true)
openssl_dep = dependency('openssl', required: true)

include_dirs = include_directories('include')

sources = files([
                    'src/ApgPackage.cpp',
                    'src/ApgLmdbDb.cpp',
                    'src/ApgArchiver.cpp',
                    'src/Logger.cpp'
                ])

headers = files([
                    'include/Apg/ApgPackage.hpp',
                    'include/Apg/Logger.hpp',
                    'include/Apg/ApgArchiver.hpp',
                    'include/Apg/ApgLmdbDb.hpp'
                ])

apg_package_lib = library('apg',
                          sources: [sources, headers],
                          include_directories: include_dirs,
                          dependencies: [nlohmann_json_dep, lmdb_dep, libarchive_dep, openssl_dep],
                          install: true,
                          version: meson.project_version()
)

apg_package_dep = declare_dependency(
    include_directories: include_dirs,
    link_with: apg_package_lib,
    dependencies: [nlohmann_json_dep]
)

install_headers(headers, subdir: 'apg_package')
