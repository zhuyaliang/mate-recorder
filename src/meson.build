gnome = import('gnome')
sources = []
gdbus_headers = []

iface =[ 
  'screen-generated', 'org.screen.', 'admin',
]

gdbus_sources = gnome.gdbus_codegen(
    iface[0],
    join_paths(data_dir, iface[1] + iface[2] + '.xml'),
    interface_prefix: iface[1],
    namespace: 'Screen',
  )
sources += gdbus_sources
gdbus_headers += gdbus_sources[1]

libscreen_generated = static_library(
  'screen-generated',
  sources: sources,
  include_directories: top_srcdir,
  dependencies: gtk_dep,
)

libscreen_generated_dep = declare_dependency(
  sources: gdbus_headers,
  include_directories: include_directories('.'),
  dependencies: gtk_dep,
  link_with: libscreen_generated,
)

sources = files(
 'main.c',
 'screen-count.c',
 'screen-save.c',
 'screen-stop.c',
 'screen-style.c',
 'screen-area.c',
 'screen-window.c',
 'screen-server.c',
)

executable('screen-admin',
  sources : sources,
  dependencies : [gtk_dep, gst_dep, indicator_dep, notify_dep, math_dep, libscreen_generated_dep],
  include_directories: top_srcdir,
  install : true,
  install_dir : get_option('bindir')
  )
