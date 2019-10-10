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
                                     dep_apitrace,
                                     dep_md5,
                                     libx11,
				                     dep_khr,
                                   ],
                                   cpp_args : '-DCMAKE_CURRENT_SOURCE_DIR="' + meson.current_source_dir() + '"'
                                  )

test('frameretrace test', frameretrace_test_exe)
