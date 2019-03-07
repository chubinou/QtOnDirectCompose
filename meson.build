project('dcomposition', 'cpp')

qt5 = import('qt5')
qt5dep = dependency('qt5', static: true, modules: ['Core','Gui','QuickControls2','Widgets','Quick','QuickWidgets'] )

moc_files = qt5.preprocess(
  moc_headers : 'compositor.hpp',
  qresources: 'resource.qrc',
  dependencies: qt5dep
)

executable(
  'DComposition',
  [ 'main.cpp', 'compositor.cpp', moc_files ],
  override_options : [
    'b_lundef=false',
    'b_asneeded=false'
  ],
  c_args : ['-gcodeview'],
  link_args: ['-Wl,-pdb,module.pdb', '-static', '-ldcomp', '-ld3d11'],
  dependencies : [
    qt5dep
  ]
)
