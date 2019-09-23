frameretrace_inc = [include_directories('.', 'context', 'gldispatch', 'amd_gpa'),
                    apitrace_inc]

subdir('context')

# TODO: extract qrenderselection from frameretrace_ui
rendering_inc = include_directories('ui', 'bargraph')

protoc = find_program('protoc', required : true)
proto_dep = dependency('protobuf', required : true, fallback : ['protobuf', 'protobuf_dep'])
proto_gen = generator(protoc, \
  output    : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
  arguments : ['--proto_path=@CURRENT_SOURCE_DIR@', '--cpp_out=@BUILD_DIR@', '@INPUT@'])

proto_generated = proto_gen.process('playback.proto')

thread_dep = dependency('threads')

subdir('gldispatch')
subdir('amd_gpa')

frameretrace_lib = static_library('frameretrace',
                                  ['glframe_batch.cpp',
                                   'glframe_batch.hpp',
                                   'glframe_cancellation.cpp',
                                   'glframe_cancellation.hpp',
                                   # 'glframe_gettime_win.cpp',
                                   'glframe_gpu_speed.hpp',
                                   'glframe_gpu_speed_linux.cpp',
                                   'glframe_logger.cpp',
                                   'glframe_logger.hpp',
                                   'glframe_metrics_amd.cpp',
                                   'glframe_metrics_amd.hpp',
                                   'glframe_metrics.cpp',
                                   'glframe_metrics.hpp',
                                   'glframe_metrics_intel.cpp',
                                   'glframe_metrics_intel.hpp',
                                   'glframe_os.hpp',
                                   'glframe_os_linux.cpp',
                                   # 'glframe_os_win.cpp',
                                   'glframe_perf_enabled.hpp',
                                   'glframe_perf_enabled_linux.cpp',
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
                                   'glframe_stderr.cpp',
                                   'glframe_stderr.hpp',
                                   'glframe_texture_override.cpp',
                                   'glframe_texture_override.hpp',
                                   'glframe_thread.hpp',
                                   'glframe_thread_linux.cpp',
                                   # 'glframe_thread_win.cpp',
                                   'glframe_traits.hpp',
                                   'glframe_uniforms.cpp',
                                   'glframe_uniforms.hpp',
                                   # 'glframe_win_shaders.cpp',
                                   'glretrace_globals.cpp',
                                   proto_generated],
                                  include_directories : [frameretrace_inc, apitrace_inc],
                                  dependencies : [
                                    dispatch_dep,
                                    libdl,
                                    libimage,
                                    libmd5,
                                    libpng,
                                    context_dep,
                                    proto_dep,
                                    thread_dep,
                                    apitrace_dep,
                                    amdgpa_dep,
                                  ])

message('apitrace inc @0@'.format(apitrace_inc))

frameretrace_dep = declare_dependency(link_with : frameretrace_lib,
                                      include_directories : [frameretrace_inc, apitrace_inc])

subdir('bargraph')
subdir('server')
subdir('ui')
subdir('framestat')
subdir('test')
