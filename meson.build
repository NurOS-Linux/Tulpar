project('tulpar', 'c',
  version: '0.1.0',
  license: 'GPL-3.0-only',
  meson_version: '>=1.4.0',
  default_options: [
    'c_std=c17',
    'warning_level=3',
    'buildtype=release',
    'b_pie=true',
  ],
)

cc = meson.get_compiler('c')

add_project_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')

if cc.has_argument('-Wformat=2')
  add_project_arguments('-Wformat=2', '-Werror=format-security', language: 'c')
endif

stack_protector_arg = cc.get_supported_arguments(['-fstack-protector-strong'])
add_project_arguments(stack_protector_arg, language: 'c')
add_project_arguments('-D_FORTIFY_SOURCE=2', language: 'c')

foreach flag : ['-Wl,-z,relro', '-Wl,-z,now']
  if cc.has_link_argument(flag)
    add_project_link_arguments(flag, language: 'c')
  endif
endforeach

sysconf_apg_dir = get_option('sysconf_apg_dir')
keyring_dir = get_option('keyring_dir')
if host_machine.system() == 'freebsd'
  if sysconf_apg_dir == '/etc/apg'
    sysconf_apg_dir = '/usr/local/etc/apg'
  endif
  if keyring_dir == '/etc/apg/trusted.d'
    keyring_dir = '/usr/local/etc/apg/trusted.d'
  endif
endif

localedir = get_option('prefix') / get_option('localedir')

add_project_arguments(
  '-DTULPAR_VERSION="' + meson.project_version() + '"',
  '-DTULPAR_SYSCONF_DIR="' + sysconf_apg_dir + '"',
  '-DTULPAR_CACHE_DIR="' + get_option('cache_dir') + '"',
  '-DTULPAR_DB_DIR="' + get_option('db_dir') + '"',
  '-DTULPAR_LOG_FILE="' + get_option('log_file') + '"',
  '-DTULPAR_REPODATA_TTL=' + get_option('repodata_ttl').to_string(),
  '-DTULPAR_MAX_PARALLEL_DOWNLOADS=' + get_option('max_parallel_downloads').to_string(),
  '-DTULPAR_KEYRING_DIR="' + keyring_dir + '"',
  '-DLOCALEDIR="' + localedir + '"',
  '-DGETTEXT_PACKAGE="tulpar"',
  language: 'c',
)

intl_dep = dependency('intl', required: false)
if not intl_dep.found()
  intl_dep = cc.find_library('intl', required: false)
endif

i18n = import('i18n')
subdir('po')

libapg_proj = subproject('libapg')
libapg_dep = libapg_proj.get_variable('libapg_dep')

curl_dep = dependency('libcurl', fallback: ['curl', 'curl_dep'])
yyjson_dep = dependency('yyjson', fallback: ['yyjson', 'yyjson_dep'])
threads_dep = dependency('threads')

tulpar_sources = files(
  'src/main.c',
  'src/cli/args.c',
  'src/cli/dispatch.c',
  'src/cli/ui.c',
  'src/config/config.c',
  'src/repo/repo.c',
  'src/repo/repodata.c',
  'src/net/http.c',
  'src/net/api.c',
  'src/net/downloader.c',
  'src/log/log.c',
  'src/util/paths.c',
  'src/util/proc.c',
  'src/cmd/cmd_common.c',
  'src/cmd/resolve.c',
  'src/cmd/cmd_install.c',
  'src/cmd/cmd_remove.c',
  'src/cmd/cmd_upgrade.c',
  'src/cmd/cmd_search.c',
  'src/cmd/cmd_list.c',
  'src/cmd/cmd_info.c',
  'src/cmd/cmd_verify.c',
  'src/cmd/cmd_orphans.c',
  'src/cmd/cmd_hold.c',
  'src/cmd/cmd_history.c',
  'src/cmd/cmd_repo.c',
  'src/cmd/cmd_download.c',
  'src/cmd/cmd_key.c',
  'src/cmd/cmd_graph.c',
)

tulpar_inc = include_directories('src')

tulpar_deps = [libapg_dep, curl_dep, yyjson_dep, threads_dep, intl_dep]

tulpar_exe = executable('tulpar',
  tulpar_sources,
  include_directories: tulpar_inc,
  dependencies: tulpar_deps,
  pie: true,
  install: true,
)

tulpar_nomain_sources = []
foreach f : tulpar_sources
  if f.full_path().endswith('src/main.c')
    continue
  endif
  tulpar_nomain_sources += f
endforeach

subdir('man')
subdir('completions')
subdir('test')
