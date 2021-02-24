actions_icons = [
  ['16x16', 'mate-recorder.png'],
  ['22x22', 'screen-start.png'],
  ['22x22', 'mate-recorder.png'],
  ['24x24', 'mate-recorder.png'],
  ['32x32', 'mate-recorder.png'],
  ['48x48', 'mate-recorder.png']
]

foreach icon: actions_icons
  install_data(
    join_paths(icon[0], 'apps', icon[1]),
    install_dir: join_paths(get_option('prefix'), 'share/icons', 'hicolor', icon[0], 'apps')
  )
endforeach
