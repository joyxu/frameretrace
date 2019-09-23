bargraph_inc = include_directories('.')

bargraph_moc = qt5.preprocess(moc_headers : ['glframe_bargraph.hpp',
                                             'glframe_qbargraph.hpp',
                                             'glframe_qselection.hpp'],
                              dependencies: qt5_dep)

bargraph_lib = static_library('bargraph', ['glframe_bargraph.cpp',
                                           'glframe_qbargraph.cpp',
                                           'glframe_qselection.cpp',
                                           bargraph_moc],
                              dependencies : [qt5_dep,
                                              frameretrace_dep],
                             include_directories : rendering_inc)

bargraph_dep = declare_dependency(link_with : bargraph_lib,
                                  include_directories : bargraph_inc)
subdir('test')
subdir('ui')
