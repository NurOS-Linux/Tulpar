tulpar_test = executable('tulpar-test',
  tulpar_nomain_sources,
  'src/main.c',
  'src/test_args.c',
  'src/test_repo.c',
  'src/test_config.c',
  'src/test_plan.c',
  'src/test_json_out.c',
  'src/test_why.c',
  'src/test_cmds.c',
  'src/test_offline.c',
  'src/test_since_spec.c',
  include_directories: tulpar_inc,
  dependencies: tulpar_deps,
  build_by_default: false,
)

test('tulpar-test', tulpar_test)
