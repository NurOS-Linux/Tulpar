tulpar_test = executable('tulpar-test',
  tulpar_nomain_sources,
  'src/main.c',
  'src/test_args.c',
  'src/test_repo.c',
  'src/test_config.c',
  'src/test_plan.c',
  include_directories: tulpar_inc,
  dependencies: tulpar_deps,
  build_by_default: false,
)

test('tulpar-test', tulpar_test)
