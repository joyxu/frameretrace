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

project('frameretrace', 'cpp', default_options : 'buildtype=debug')

add_global_arguments([
                      '-fno-rtti',
                      '-DGOOGLE_PROTOBUF_NO_RTTI',
                      '-fno-exceptions',
                    ], language : 'cpp')

qt5 = import('qt5')
qt5_dep = dependency('qt5', modules: [
                       'Core',
                       'Gui',
                       'Qml',
                       'Quick',
                       'Widgets',
                       'OpenGL',
                       'Concurrent',
                     ])

apitrace_target = run_target('apitrace',
                             command : ['python3', 'subprojects/build_apitrace.py'])

cpp = meson.get_compiler('cpp')
libdl = cpp.find_library('dl', required : true)
libpng = cpp.find_library('png', required : true)
libwaffle = cpp.find_library('waffle-1', required : true)
libx11 = cpp.find_library('X11', required : true)
libz = cpp.find_library('z', required : true)
libprocps = cpp.find_library('procps', required : true)

libglretrace_common = cpp.find_library('glretrace_common',
                                       dirs : meson.source_root() / 'subprojects/apitrace/build/retrace',
                                       required : false)
libretrace_common = cpp.find_library('retrace_common',
                                     dirs : meson.source_root() / 'subprojects/apitrace/build/retrace',
                                     required : false)
libcommon = cpp.find_library('common',
                             dirs : meson.source_root() / 'subprojects/apitrace/build/lib/trace',
                             required : false)
libmd5 = cpp.find_library('md5_bundled',
                          dirs : meson.source_root() / 'subprojects/apitrace/build/thirdparty/md5',
                          required : false)
libimage = cpp.find_library('image',
                            dirs : meson.source_root() / 'subprojects/apitrace/build/lib/image',
                            required : false)
libglhelpers = cpp.find_library('glhelpers',
                                dirs : meson.source_root() / 'subprojects/apitrace/build/helpers',
                                required : false)
libglproc = cpp.find_library('glproc',
                              dirs : meson.source_root() / 'subprojects/apitrace/build/dispatch',
                              required : false)
libglproc_gl = cpp.find_library('glproc_gl',
                              dirs : meson.source_root() / 'subprojects/apitrace/build/dispatch',
                              required : false)
libhighlight = cpp.find_library('highlight',
                                dirs : meson.source_root() / 'subprojects/apitrace/build/lib/highlight',
                                required : false)
libos = cpp.find_library('os',
                                dirs : meson.source_root() / 'subprojects/apitrace/build/lib/os',
                                required : false)
libguids = cpp.find_library('guids',
                                dirs : meson.source_root() / 'subprojects/apitrace/build/lib/guids',
                                required : false)
libtrace = cpp.find_library('trace',
                                dirs : meson.source_root() / 'subprojects/apitrace/build/lib/trace',
                                required : false)
libbrotli = cpp.find_library('brotli_common',
                                dirs : meson.source_root() / 'subprojects/apitrace/build/thirdparty/brotli',
                                required : false)
libbrotli_enc = cpp.find_library('brotli_enc',
                                dirs : meson.source_root() / 'subprojects/apitrace/build/thirdparty/brotli',
                                required : false)
libbrotli_dec = cpp.find_library('brotli_dec',
                                dirs : meson.source_root() / 'subprojects/apitrace/build/thirdparty/brotli',
                                required : false)
libsnappy = cpp.find_library('snappy_bundled',
                                dirs : meson.source_root() / 'subprojects/apitrace/build/thirdparty/snappy',
                                required : false)
libbacktrace = cpp.find_library('backtrace',
                                dirs : meson.source_root() / 'subprojects/apitrace/build/thirdparty/libbacktrace',
                                required : false)

apitrace_inc = include_directories('subprojects/apitrace/retrace',
                                   'subprojects/apitrace/lib/image',
                                   'subprojects/apitrace/lib/trace',
                                   'subprojects/apitrace/lib/os',
                                   'subprojects/apitrace/helpers',
                                   'subprojects/apitrace/retrace',
                                   'subprojects/apitrace/dispatch',
                                   'subprojects/apitrace/thirdparty/khronos',
                                   'subprojects/apitrace/build/dispatch',
                                   'subprojects/apitrace/thirdparty/md5')

apitrace_dep = declare_dependency(dependencies : [
                                   libcommon,
                                   libbrotli,
                                   libbrotli_enc,
                                   libbrotli_dec,
                                   libglhelpers,
                                   libsnappy,
                                   libbacktrace,
                                   libtrace,
                                   libprocps,
                                   libos,
                                   libhighlight,
                                   libguids,
                                   libglproc,
                                   libglproc_gl,
                                   libglretrace_common,
                                   libretrace_common,
                                   libx11,
                                   libz,
                                 ],
                                  include_directories : apitrace_inc)

gtest_dep = dependency('gtest', fallback : ['gtest', 'gtest_dep'])

subdir('src')
