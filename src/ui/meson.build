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

frameretrace_ui_inc = include_directories('.')
frameretrace_ui_dep = declare_dependency(include_directories : frameretrace_inc)

frameretrace_moc = qt5.preprocess(moc_headers : ['glframe_api_model.hpp',
                                                 'glframe_batch_model.hpp',
                                                 'glframe_experiment_model.hpp',
                                                 'glframe_metrics_model.hpp',
                                                 'glframe_qutil.hpp',
                                                 'glframe_rendertarget_model.hpp',
                                                 'glframe_retrace_images.hpp',
                                                 'glframe_retrace_model.hpp',
                                                 'glframe_shader_model.hpp',
                                                 'glframe_state_model.hpp',
                                                 'glframe_texture_model.hpp',
                                                 'glframe_uniform_model.hpp',
                                                ],
                              dependencies: qt5_dep)

frameretrace_qrc = qt5.preprocess(qresources : 'resources.qrc')

cpp = meson.get_compiler('cpp')

frameretrace_exe = executable('frameretrace',
                              [frameretrace_moc,
                               frameretrace_qrc,
                               'glframe_api_model.cpp',
                               'glframe_batch_model.cpp',
                               'glframe_experiment_model.cpp',
                               'glframe_metrics_model.cpp',
                               'glframe_qutil.cpp',
                               'glframe_rendertarget_model.cpp',
                               'glframe_retrace_images.cpp',
                               'glframe_retrace_model.cpp',
                               'glframe_shader_model.cpp',
                               'glframe_state_model.cpp',
                               'glframe_texture_model.cpp',
                               'glframe_uniform_model.cpp',
                               'main.cpp'],
                              dependencies : [
                                bargraph_dep,
                                frameretrace_dep,
                                dep_apitrace,
                                qt5_dep,
                                libx11,
				                dep_khr,
				                win_deps,
				                protobuf_dep,
                              ],
                              include_directories : rendering_inc,
                              cpp_args : '-DFRAMERETRACE_BUILD_DIR="' + server_dir + '"',
                             install : true)
