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
                                              frameretrace_dep,
                                              dep_apitrace],
                             include_directories : rendering_inc)

bargraph_dep = declare_dependency(link_with : bargraph_lib,
	     include_directories : bargraph_inc)

if libwaffle.found()
  subdir('test')
else
  bargraphtest_dep = dependency('', required : false)
endif
subdir('ui')
