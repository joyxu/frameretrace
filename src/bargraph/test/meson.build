src = files('main_test.cpp',
            'test_bargraph.cpp',
            'test_bargraph_ctx.cpp',
            'test_bargraph_ctx.hpp',
            'test_selection.cpp',
            'test_selection.hpp')

bargraphtest_inc = include_directories('.')
bargraphtest_moc = qt5.preprocess(moc_headers : ['test_bargraph_ctx.hpp',
                                                 'test_selection.hpp',],
                                  dependencies: qt5_dep)

exe = executable('bargraph_test', [src, bargraphtest_moc],
                 dependencies : [dispatch_dep,
                                 libwaffle,
                                 bargraph_dep,
                                 gtest_dep,
                                 frameretrace_dep,
                                 qt5_dep])

bargraphtest_lib = static_library('bargraph_test',
                                  'test_bargraph_ctx.cpp',
                                  dependencies : frameretrace_dep,
                                 )
bargraphtest_dep = declare_dependency(link_with : bargraphtest_lib,
                                      include_directories : '.')

test('bar graph test', exe)
