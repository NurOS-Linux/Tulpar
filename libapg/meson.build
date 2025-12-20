# NurOS Ruzen42 2025

libapg_sources = files(
  'src/package.c'
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
    libarchive_dep,
  ],
  install: true,
)

libapg_dep = declare_dependency(
  link_with: libapg,
  include_directories: libapg_inc,
)
