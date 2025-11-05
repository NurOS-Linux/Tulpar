# NurOS Ruzen42 2025

libapg_sources = files(
  'src/ApgArchiver.cpp',
  'src/ApgLmdbDb.cpp',
  'src/ApgPackage.cpp',
  'src/Logger.cpp',
  'src/Md5Hash.cpp',
)

libapg_inc = include_directories('include')

libarchive_dep = dependency('libarchive', required: true)
if not libarchive_dep.found()
  cpp = meson.get_compiler('cpp')
  libarchive_dep = cpp.find_library('archive', required: true)
endif

libapg = library('apg', libapg_sources,
  include_directories: libapg_inc,
  dependencies: [
    dependency('openssl'),
    dependency('lmdb', required: true),
    dependency('nlohmann_json', required: true),
    libarchive_dep,
  ],
  install: true,
)

libapg_dep = declare_dependency(
  link_with: libapg,
  include_directories: libapg_inc,
)
