project('template_meson', 'cpp',
  version : '0.1.0',
  default_options : [
    'cpp_std=c++23',
    'warning_level=2',
    'default_library=static',
  ]
)

# first slang

slangc = find_program('slangc')

vert_spv = custom_target('rect.vert.spv',
  input  : 'shaders/rect.slang',
  output : 'rect.vert.spv',
  command : [slangc, '@INPUT@', '-entry', 'vertMain', '-stage', 'vertex', '-target', 'spirv', '-o', '@OUTPUT@']
)

frag_spv = custom_target('rect.frag.spv',
  input  : 'shaders/rect.slang',
  output : 'rect.frag.spv',
  command : [slangc, '@INPUT@', '-entry', 'fragMain', '-stage', 'fragment', '-target', 'spirv', '-o', '@OUTPUT@']
)

fmt_dep = dependency('fmt', fallback : ['fmt', 'fmt_dep'])
sdl3_dep = dependency('SDL3', fallback : ['sdl3', 'sdl3_dep'])
glm_dep = dependency('glm', fallback : ['glm', 'glm_dep'])

# ── Core ──────────────────────────────────────────────────────────────────────
core_inc = include_directories('root/core/inc')

core_sources = files(
  # add your .cpp files here or useu:
  'root/core/src/graphics.cpp',
  'root/core/src/LLGraphics.cpp'
)

core_lib = static_library('core',
  sources : core_sources,
  include_directories : core_inc,
  dependencies : [fmt_dep, sdl3_dep, glm_dep],
)

core_dep = declare_dependency(
  link_with : core_lib,
  include_directories : core_inc,
  dependencies : [fmt_dep, sdl3_dep, glm_dep],
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

