project('tulpar', 'cpp',
  version: '0.1.1',
  default_options: ['warning_level=3', 'cpp_std=c++17'])

# Find dependencies
openssl_dep = dependency('libcrypto', required: true)  # Only libcrypto is needed for MD5
json_dep = dependency('nlohmann_json', required: true)  # For nlohmann/json

# Define include directories
inc_dirs = include_directories('utils', 'parse_json')

executable('tulpar',
           sources: [
             'main.cpp',
             'utils/utils.cpp',
             'parse_json/parse_json.cpp'
           ],
           include_directories: inc_dirs,
           dependencies: [openssl_dep, json_dep],
           install: true)
