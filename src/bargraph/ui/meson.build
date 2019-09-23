bargraph_qrc = qt5.preprocess(qresources : 'resources.qrc')

executable('bargraph',
           ['main.cpp', bargraph_qrc],
           dependencies : [dispatch_dep,
                           bargraph_dep,
                           frameretrace_dep,
                           qt5_dep])
