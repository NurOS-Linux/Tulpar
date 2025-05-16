project('tulpar', 'cpp',
  version: '0.1.0',
  default_options: ['warning_level=1', 'cpp_std=c++17'])

openssl_dep = dependency('libcrypto', required: true)  
json_dep = dependency('nlohmann_json', required: true)

inc_dirs = include_directories('utils', 'parse_json')

executable('tulpar',
           sources: [
             'main.cpp',
             'utils/utils.cpp',
             'parse_json/parse_json.cpp'
           ],
           include_directories: inc_dirs,
           dependencies: [openssl_dep, json_dep],
           install: true,
           cpp_args: [
                      '-O0',  
                      '-D_GLIBCXX_ASSERTIONS=1',
                      '-D_FILE_OFFSET_BITS=64',
                     ],)
