api_version = '0.1'

nesbrasa_sources = [
    'cpu.c',
    'instrucao.c',
    'memoria.c',
    'nesbrasa.c',
    'ppu.c',
]

nesbrasa_headers = [
  'cpu.h',
  'instrucao.h',
  'memoria.h',
  'nesbrasa.h',
  'ppu.h',
]


nesbrasa_deps = [

]

install_headers(nesbrasa_headers, subdir: 'nesbrasa')

nesbrasa_lib = shared_library('nesbrasa' + api_version,
  nesbrasa_sources,
  include_directories : inc,
  dependencies: nesbrasa_deps,
  install: true,
)

pkg = import('pkgconfig')

pkg.generate(
  description: 'Um emulador de NES',
    libraries: nesbrasa_lib,
         name: 'braza-nes',
     filebase: 'nesbrasa' + api_version,
      version: meson.project_version(),
      subdirs: 'nesbrasa',
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)
