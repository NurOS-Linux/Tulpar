project('apglist', 'cpp',
        version : '0.1.0',
        default_options : [
            'warning_level=3',
            'cpp_std=c++23',
            'buildtype=release'
        ]
)

apg_dep = dependency('libapg', required: true)

exe = executable('apglist', 'src/main.cpp',
                 dependencies: [apg_dep],
                 install : true)