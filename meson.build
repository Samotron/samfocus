project('samfocus', 'c', 'cpp',
  version: '0.1.0',
  default_options: [
    'c_std=c11',
    'cpp_std=c++11',
    'warning_level=2',
    'buildtype=debug'
  ]
)

# Platform detection
host_system = host_machine.system()
is_windows = host_system == 'windows'
is_linux = host_system == 'linux'

# Dependencies
glfw_dep = dependency('glfw3', required: false)
if not glfw_dep.found()
  # Fall back to system package or fail with helpful message
  glfw_dep = dependency('glfw3', 
    required: true,
    fallback: ['glfw', 'glfw_dep'])
endif

gl_dep = dependency('gl', required: false)
if not gl_dep.found()
  # On Windows, OpenGL comes with the system
  if is_windows
    gl_dep = meson.get_compiler('c').find_library('opengl32', required: true)
  else
    gl_dep = dependency('gl', required: true)
  endif
endif

sqlite_dep = dependency('sqlite3', required: true)

# Platform-specific dependencies
platform_deps = []
if is_linux
  platform_deps += dependency('dl', required: false)
  platform_deps += dependency('threads', required: true)
  platform_deps += dependency('x11', required: true)
elif is_windows
  # Windows needs these for SHGetFolderPath and file operations
  platform_deps += meson.get_compiler('c').find_library('shell32', required: true)
  platform_deps += meson.get_compiler('c').find_library('user32', required: true)
endif

# cimgui sources
cimgui_inc = include_directories('external/cimgui', 'external/cimgui/imgui')

cimgui_sources = files(
  'external/cimgui/cimgui.cpp',
  'external/cimgui/imgui/imgui.cpp',
  'external/cimgui/imgui/imgui_draw.cpp',
  'external/cimgui/imgui/imgui_tables.cpp',
  'external/cimgui/imgui/imgui_widgets.cpp',
  'external/cimgui/imgui/imgui_demo.cpp',
  'external/cimgui/imgui/backends/imgui_impl_glfw.cpp',
  'external/cimgui/imgui/backends/imgui_impl_opengl3.cpp',
)

# Build cimgui as a static library
cimgui_lib = static_library('cimgui',
  cimgui_sources,
  include_directories: cimgui_inc,
  dependencies: [glfw_dep, gl_dep],
  cpp_args: ['-DIMGUI_IMPL_API=extern "C"']
)

cimgui_dep = declare_dependency(
  link_with: cimgui_lib,
  include_directories: cimgui_inc,
  dependencies: [glfw_dep, gl_dep]
)

# Application sources
src_inc = include_directories('src')

app_sources = files(
  'src/main.c',
  'src/core/platform.c',
  'src/core/task.c',
  'src/db/database.c',
  'src/ui/inbox_view.c',
)

# Platform-specific compile args
platform_args = []
if is_linux
  platform_args += ['-DPLATFORM_LINUX']
elif is_windows
  platform_args += ['-DPLATFORM_WINDOWS', '-D_CRT_SECURE_NO_WARNINGS']
endif

# Main executable
executable('samfocus',
  app_sources,
  include_directories: src_inc,
  dependencies: [cimgui_dep, sqlite_dep] + platform_deps,
  c_args: platform_args,
  install: true
)
