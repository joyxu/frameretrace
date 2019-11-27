# Copyright © 2019 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

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
                                 libx11,
                                 libwaffle,
                                 bargraph_dep,
                                 gtest_dep,
                                 frameretrace_dep,
                                 protobuf_dep,
                                 qt5_dep])

bargraphtest_lib = static_library('bargraph_test',
                                  'test_bargraph_ctx.cpp',
                                  dependencies : frameretrace_dep,
                                 )
bargraphtest_dep = declare_dependency(link_with : bargraphtest_lib,
                                      include_directories : '.')

test('bar graph test', exe)
