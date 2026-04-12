project('template_meson', ['c', 'cpp'],
  version : '0.1.0',
  default_options : [
    'cpp_std=c++23',
    'warning_level=2',
    'default_library=static',
  ]
)

# first slang


# ── Core ──────────────────────────────────────────────────────────────────────
core_inc = include_directories('root/core/inc')

core_sources = files(
  # add your .cpp files here or useu:
  'root/core/src/render/graphics.cpp',
  'root/core/src/render/LLGraphics.cpp',
  'root/core/src/render/libs.cpp',
  'root/core/src/render/object.cpp',
  'root/core/src/render/LLObject.cpp',
  'root/core/src/render/low/texture.cpp',
  'root/core/src/render/low/mesh.cpp',
  'root/core/src/render/low/mikktspace.c',
  'root/core/src/render/low/objMTS.cpp'
)

core_lib = static_library('core',
  sources : core_sources,
  include_directories : core_inc,
  dependencies : [fmt_dep, sdl3_dep, glm_dep, stb_dep],
)

core_dep = declare_dependency(
  link_with : core_lib,
  include_directories : core_inc,
  dependencies : [fmt_dep, sdl3_dep, glm_dep, stb_dep],
)

# ── App ───────────────────────────────────────────────────────────────────────
app_inc = include_directories('root/app/inc')

app_sources = files(
  # add your .cpp files here
  'root/app/src/main.cpp'
)

executable('app',
  sources : app_sources,
  include_directories : app_inc,
  dependencies : [core_dep],
)

