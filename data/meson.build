subdir('counter')
subdir('icon')
subdir('hicolor')

i18n = import('i18n')

ifaces = files(
  screen_namespace + '.xml',
)
install_data(
  ifaces,
  install_dir: dbus_ifaces_dir,
)

i18n.merge_file(
  'mate-recorder-desktop',
  input: 'mate-recorder.desktop.in',
  output: 'mate-recorder.desktop',
  install: true,
  po_dir: join_paths(meson.source_root(), 'po'),
  install_dir : join_paths(get_option('prefix'), 'share/applications'),
  type: 'desktop',
)

appdata = 'mate-recorder.appdata.xml'
i18n.merge_file(
  input: appdata + '.in',
  output: appdata,
  po_dir: join_paths(meson.source_root(), 'po'),
  install: true,
  install_dir: join_paths(get_option('prefix'), 'share/metainfo')
)  
