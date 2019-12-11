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
### Debian
Minimal packages required to build frameretrace:
```
sudo apt install build-essential libprocps-dev meson ninja-build python3-setuptools qml-module-qt-labs-folderlistmodel qml-module-qt-labs-settings qml-module-qtquick-controls qml-module-qtquick-controls2 qml-module-qtquick-dialogs qtbase5-dev qtbase5-dev qtdeclarative5-dev zlib1g-dev libpng-dev ccache

```
### Windows
#### System Setup
##### Qt
Install Qt, with either the mingw toolset or MSVC (or both), depending
on your available tools.

Add c:\Qt\{version}\{toolkit}\bin to your systems PATH, for each
toolkit that you want to use.
##### Chocolatey
From a shell with administrator access:
```
 > choco install mingw python3 git ninja pkgconfiglite -y
 
```
##### Meson
From a shell with administrator access:
```
 > py -m pip install meson
 
```
#### Compilation
Meson wraps have not been published for some of the Apitrace 3rd party
libraries.  For Linux, the libraries are symlinked into
frameretrace/subprojects.  For Windows, we must delete the symlinks
and copy the source files
frameretrace/subprojects/apitrace/thirdparty.

For mingw:
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

For Visual Studio, run the same commands in a Visual Studio Native
Tools Command Prompt.  NOTE: Meson 0.52 has a bug which causes Visual
Studio projects to add glproc.hpp paths where a directory path is
needed.  If this hasn't already been fixed, you can edit the project
files that fail to load and remove the glproc.hpp filename from each
entry that should configure an include path.

#### Installation

Use the prefix argument at meson setup time to create a distributable
directory of FrameRetrace executables and DLLs:
```
 > meson setup --prefix={full_src_path}/install build
 > cd build
 > ninja install
```
After building, use Qt's deployment tool to add Qt DLL's to the
installation path.
```
 > c:/Qt/{version}/{mingw_ver}/bin/windeployqt.exe --qmldir {src_dir}/src/ui/qml {install_dir}/bin/frameretrace.exe
```
If mingw is not installed on your target system, you can copy the DLLs
from the mingw installation path to the executable directory.
Chocolatey's mingw installation path is:

C:/ProgramData/chocolatey/lib/mingw/tools/install/mingw64/bin

You should be able to copy {install_dir} to a target machine and run
it with no other run-time requirements.