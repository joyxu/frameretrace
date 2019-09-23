frameretrace_test_exe = executable('frameretrace_test',
                                   [
                                     'main_test.cpp',
                                     'retrace_daemon_test.cpp',
                                     'retrace_file_transfer_test.cpp',
                                     'retrace_log_test.cpp',
                                     'retrace_metrics_test.cpp',
                                     'retrace_socket_test.cpp',
                                     'retrace_thread_test.cpp',
                                   ],
                                   dependencies : [
                                     frameretrace_dep,
                                     libwaffle,
                                     bargraphtest_dep,
                                     gtest_dep,
                                   ],
                                   cpp_args : '-DCMAKE_CURRENT_SOURCE_DIR="' + meson.current_source_dir() + '"'
                                  )

test('frameretrace test', frameretrace_test_exe)
