bash_comp = dependency('bash-completion', required: false)
if bash_comp.found()
  bashcompdir = bash_comp.get_variable(pkgconfig: 'completionsdir')
else
  bashcompdir = get_option('datadir') / 'bash-completion' / 'completions'
endif

install_data('bash/tulpar',
  install_dir: bashcompdir,
  rename: 'tulpar',
)

zsh_comp = dependency('zsh', required: false)
if zsh_comp.found()
  zshcompdir = zsh_comp.get_variable(pkgconfig: 'completionsdir')
else
  zshcompdir = get_option('datadir') / 'zsh' / 'site-functions'
endif

install_data('zsh/_tulpar', install_dir: zshcompdir)

fish_comp = dependency('fish', required: false)
if fish_comp.found()
  fishcompdir = fish_comp.get_variable(pkgconfig: 'completionsdir')
else
  fishcompdir = get_option('datadir') / 'fish' / 'vendor_completions.d'
endif

install_data('fish/tulpar.fish', install_dir: fishcompdir)
