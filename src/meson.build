api_version = '0.1'

nesbrasa_sources = [
    'cartucho.cpp',
    'cpu.cpp',
    'instrucao.cpp',
    'mapeadores/nrom.cpp',
    'memoria.cpp',
    'nesbrasa.cpp',
    'ppu.cpp',
    'util.cpp',
]

nesbrasa_headers = [
  'cartucho.hpp',
  'cpu.hpp',
  'instrucao.hpp',
  'mapeadores/nrom.hpp',
  'memoria.hpp',
  'nesbrasa.hpp',
  'ppu.hpp',
  'util.hpp',
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
