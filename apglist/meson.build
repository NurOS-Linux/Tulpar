project('apglist', 'cpp',
        version : '0.1.0',
        default_options : [
            'warning_level=3',
            'cpp_std=c++23',
            'buildtype=release'
        ]
)

apg_dep = dependency('libapg', required: true)
lmdb_dep = dependency('lmdb', required: true)

exe = executable('apglist', 'src/main.cpp',
                 dependencies: [apg_dep, lmdb_dep],
                 install : true)