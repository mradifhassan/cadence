project('cadence', 'c',
  version: '1.0.0',
  license: 'GPL-3.0-only',
  default_options: ['warning_level=2', 'c_std=gnu11'])

cc = meson.get_compiler('c')

deps = [
  dependency('gtk4', version: '>= 4.6'),
  dependency('pangocairo'),
  dependency('sdl2'),
  cc.find_library('m', required: false),
]

datadir = get_option('prefix') / get_option('datadir') / 'cadence'
icondir = get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps'
desktopdir = get_option('prefix') / get_option('datadir') / 'applications'

executable('cadence', 'src/main.c',
  dependencies: deps,
  c_args: ['-DCADENCE_DATADIR="@0@"'.format(datadir)],
  install: true)

# Optional custom key sound; the app falls back to a built-in click if it is missing.
install_data('assets/click.wav', install_dir: datadir)

# Desktop file for the application menu
install_data('cadence.desktop', install_dir: desktopdir)

# Application icon (scalable PNG used as desktop icon)
install_data('assets/typing-board.png', install_dir: icondir, rename: 'cadence.png')