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

frameretrace_inc = [include_directories('.', 'context', 'gldispatch', 'amd_gpa')]

subdir('gldispatch')
subdir('context')

rendering_inc = include_directories('ui', 'bargraph')

proto_gen = generator(protoc, \
  output    : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
  arguments : ['--proto_path=@CURRENT_SOURCE_DIR@', '--cpp_out=@BUILD_DIR@', '@INPUT@'])

proto_generated = proto_gen.process('playback.proto')

thread_dep = dependency('threads')

subdir('amd_gpa')

win_deps = []
if host_machine.system() == 'windows'
  os_sources = ['glframe_os_win.cpp',
                'glframe_thread_win.cpp',
                'glframe_win_shaders.cpp',
	       ]
  win_deps += [cpp.find_library('ws2_32'), # winsock
	       cpp.find_library('Opengl32'), #opengl
	       ]
else
  os_sources = ['glframe_gpu_speed_linux.cpp',
                 'glframe_os_linux.cpp',
                 'glframe_perf_enabled_linux.cpp',
                 'glframe_thread_linux.cpp',
		 'glframe_stderr.cpp',
		 'glframe_batch.cpp',
		]
endif

if cpp.get_argument_syntax() == 'msvc' or host_machine.system() == 'linux'
  mdapi_files = ['glframe_metrics_mdapi.cpp',
                 'glframe_metrics_mdapi.hpp',
		]
else
  mdapi_files = []
endif

frameretrace_lib = static_library('frameretrace',
                                  [os_sources,
				                   mdapi_files,
                                   'glframe_batch.hpp',
                                   'glframe_cancellation.cpp',
                                   'glframe_cancellation.hpp',
                                   'glframe_gpu_speed.hpp',
                                   'glframe_logger.cpp',
                                   'glframe_logger.hpp',
                                   'glframe_metrics_amd.cpp',
                                   'glframe_metrics_amd.hpp',
                                   'glframe_metrics.cpp',
                                   'glframe_metrics.hpp',
                                   'glframe_metrics_intel.cpp',
                                   'glframe_metrics_intel.hpp',
                                   'glframe_os.hpp',
                                   'glframe_perf_enabled.hpp',
                                   'glframe_retrace_context.cpp',
                                   'glframe_retrace_context.hpp',
                                   'glframe_retrace.cpp',
                                   'glframe_retrace.hpp',
                                   'glframe_retrace_interface.hpp',
                                   'glframe_retrace_render.cpp',
                                   'glframe_retrace_render.hpp',
                                   'glframe_retrace_skeleton.cpp',
                                   'glframe_retrace_skeleton.hpp',
                                   'glframe_retrace_stub.cpp',
                                   'glframe_retrace_stub.hpp',
                                   'glframe_retrace_texture.cpp',
                                   'glframe_retrace_texture.hpp',
                                   'glframe_socket.cpp',
                                   'glframe_socket.hpp',
                                   'glframe_state.cpp',
                                   'glframe_state_enums.cpp',
                                   'glframe_state_enums.hpp',
                                   'glframe_state.hpp',
                                   'glframe_state_override.cpp',
                                   'glframe_state_override.hpp',
                                   'glframe_stderr.hpp',
                                   'glframe_texture_override.cpp',
                                   'glframe_texture_override.hpp',
                                   'glframe_thread.hpp',
                                   'glframe_traits.hpp',
                                   'glframe_uniforms.cpp',
                                   'glframe_uniforms.hpp',
                                   'glretrace_globals.cpp',
                                   proto_generated],
                                  include_directories : [frameretrace_inc],
                                  dependencies : [
                                    dispatch_dep,
                                    libdl,
                                    dep_md5,
                                    libpng,
                                    context_dep,
                                    protobuf_dep,
                                    thread_dep,
                                    dep_apitrace,
                                    amdgpa_dep,
                                    dep_apitrace,
                                    dep_mdapi,
                                    dep_khr,
                                  ])

frameretrace_dep = declare_dependency(link_with : frameretrace_lib,
                                      include_directories : [frameretrace_inc])

subdir('bargraph')
subdir('server')
subdir('ui')
subdir('test')
subdir('framestat')
subdir('framemetrics')


