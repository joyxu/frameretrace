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

cpp = meson.get_compiler('cpp')
libdl = cpp.find_library('dl', required : false)
libpng = dependency('libpng', required : true, fallback :['libpng', 'png_dep'])
libwaffle = dependency('waffle', required : true, fallback :['waffle', 'ext_waffle'])
libx11 = cpp.find_library('X11', required : false)
libz = dependency('zlib', required : true, fallback : ['zlib', 'zlib_dep'])

proj_apitrace = subproject('apitrace')

dep_apitrace = proj_apitrace.get_variable('dep_apitrace')

gtest_dep = dependency('gtest', fallback : ['gtest', 'gtest_dep'])

#cmake = import('cmake')
add_project_arguments('-DHAVE_BACKTRACE=0', language : 'cpp')
dep_md5 = subproject('md5').get_variable('dep_md5')
dep_brotli_enc = dependency('libbrotlienc', fallback : ['brotli', 'dep_brotli_encoder'])
dep_brotli_dec = dependency('libbrotlidec', fallback : ['brotli', 'dep_brotli_decoder'])
dep_khr = subproject('khronos').get_variable('dep_khr')

subdir('src')
