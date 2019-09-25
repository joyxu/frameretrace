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

## Build

```
 $ meson subprojects download
 $ meson setup build
 $ ninja -C build apitrace
 $ ninja -C build
```
