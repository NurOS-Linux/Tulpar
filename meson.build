project('tulpar', 'c',
        version : '0.1.0',
        default_options : [
            'warning_level=3',
            'cpp_std=c++23',
            'buildtype=release'
        ]
)

argparse_dep = dependency('argparse', required: true)

subdir('libapg')
subdir('apginstall')
subdir('apgremove')
subdir('apglist')
