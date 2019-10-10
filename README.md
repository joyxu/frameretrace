# FrameRetrace gpu performance analyzer and debugger

FrameRetrace is a UI for apitrace which loops over a set of frames in
a trace file.  It collects GPU metrics, render targets, shader
information, and OpenGL state, and enables the user to modify the
frame by editing shaders or other live experiments.

## History

For many years, FrameRetrace lived as a subdirectory within a fork of
apitrace.  Due to size, complexity, and lack of interest from the
apitrace maintainer, FrameRetrace will not be merged upstream as a UI
for apitrace.

Rather than constantly rebase to take advantage of new apitrace
features, FrameRetrace will be restructured to make use of apitrace as
an external project.

### Starting points

This repo was generated from the following git commands:

```
 $ git filter-branch --subdirectory-filter retrace/daemon -- --all
 $ git filter-branch --index-filter   'git ls-files -s | sed "s-\t\"*-&src/-" |
   GIT_INDEX_FILE=$GIT_INDEX_FILE.new \
   git update-index --index-info &&
   mv "$GIT_INDEX_FILE.new" "$GIT_INDEX_FILE"' HEAD
```

## Goals

1. Minimize patches in apitrace fork that exposes api for retrace
1. Create a meson build system for FrameRetrace to compile and link
1. externals support to pull apitrace fork through meson
1. Implement/verify/minimize build support

## Meson for apitrace

apitrace subprojects needed to be linked up to subprojects/

Because of cmake bugs, this did not work:

```
 $ meson wrap promote subprojects/apitrace/thirdparty/brotli
```

Dylan Baker helped me add minimal meson build support for Apitrace,
allowing FrameRetrace to use it as a native meson subproject.  Most of
the 3rdparty dependencies of Apitrace could be build with meson's
cmake support.

## Build

```
 $ meson setup build
 $ ninja -C build
```

### Windows
#### System Setup
From a shell with administrator access:
```
 > choco install pkgconfiglite
 > choco install getopt
```
#### Compilation
Meson wraps have not been published for some of the Apitrace 3rd party libraries.  For Linux, the libraries are symlinked into frameretrace/subprojects.  For Windows, we must delete the symlinks and copy the source files frameretrace/subprojects/apitrace/thirdparty.

From a Visual Studio Native Tools Command Prompt
```
 > meson subprojects download 
 > del subprojects\snappy
 > xcopy  subprojects\apitrace\thirdparty\snappy subprojects\snappy /s /e /h /I
 > del subprojects\md5
 > xcopy  subprojects\apitrace\thirdparty\md5 subprojects\md5 /s /e /h /I
 > del subprojects\brotli
 > xcopy  subprojects\apitrace\thirdparty\brotli subprojects\brotli /s /e /h /I
 > del subprojects\khronos
 > xcopy  subprojects\apitrace\thirdparty\khronos subprojects\khronos /s /e /h /I
 > del subprojects\crc32c
 > xcopy  subprojects\apitrace\thirdparty\crc32c subprojects\crc32c /s /e /h /I
 > xcopy  subprojects\apitrace\thirdparty\mhook subprojects\mhook /s /e /h /I
 > xcopy  subprojects\apitrace\thirdparty\getopt subprojects\getopt /s /e /h /I
 > meson setup build
 > cd build
 > ninja
```
