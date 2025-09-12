#!/bin/sh

set -e

SRC="main.c microui.c glad.c render/gl1.c render/gl3.c render/math.c render/overlay.c util/cpu.c util/mem.c"
STDFLAGS="-std=c99 -shared -fPIC -Wall"

OUTPUT="libsimpleoverlay.so"
OUTPUT_32="libsimpleoverlay32.so"

WRAPPER="simpleoverlay"

TARGET="$1"

if [ $(uname) != "Darwin" ]; then
		SRC="$SRC hooks/dlsym.c hooks/egl.c hooks/glx.c hooks/vulkan.c util/elfhacks.c util/amdgpu.c"
else
		SRC="$SRC hooks/cgl.c util/applegpu.c"
		STDFLAGS="$STDFLAGS -framework Foundation -framework IOKit"
		OUTPUT="libsimpleoverlay.dylib"
fi

if [ "$TARGET" = "" ]; then
		if [ $(uname) != "Darwin" ]; then
			set -x

			cc $SRC $STDFLAGS -o $OUTPUT
		else
			set -x

			cc $SRC $STDFLAGS -arch x86_64 -o amd64.dylib
			cc $SRC $STDFLAGS -arch arm64 -o arm64.dylib
			lipo -create -output $OUTPUT amd64.dylib arm64.dylib
			rm amd64.dylib arm64.dylib
		fi
elif [ "$TARGET" = "multilib" ]; then
		set -x
		
		cc $SRC -m32 $STDFLAGS -o $OUTPUT_32
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
	
