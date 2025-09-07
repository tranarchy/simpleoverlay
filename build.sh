#!/bin/sh

set -e

SRC="main.c microui.c glad.c render/gl.c render/overlay.c util/amdgpu.c util/cpu.c util/mem.c"
STDFLAGS="-std=c99 -shared -fPIC -O2 -Wall"

OUTPUT="libsimpleoverlay.so"
OUTPUT_32="libsimpleoverlay32.so"

WRAPPER="simpleoverlay"

TARGET="$1"

if [ $(uname) != "Darwin" ]; then
		SRC="$SRC hooks/dlsym.c hooks/egl.c hooks/glx.c hooks/vulkan.c util/elfhacks.c"
else
		SRC="$SRC hooks/cgl.c"
		STDFLAGS="$STDFLAGS -framework Foundation"
		OUTPUT="libsimpleoverlay.dylib"
fi

if [ "$TARGET" = "" ]; then
		set -x
		
		cc $SRC $STDFLAGS -o $OUTPUT
elif [ "$TARGET" = "multilib" ]; then
		set -x
		
		cc $SRC -m32 $STDFLAGS -o $OUTPUT
elif [ "$TARGET" = "install" ]; then
		set -x
		
		mkdir -p /usr/local/bin
		chmod +x $WRAPPER
		cp -f $WRAPPER /usr/local/bin

		mkdir -p /usr/local/lib
		cp -f libsimpleoverlay.* /usr/local/lib
elif [ "$TARGET" = "install_multilib" ]; then
		set -x
		
		cp -f $OUTPUT_32 /usr/lib32/$OUTPUT
fi
	
