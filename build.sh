#!/bin/sh

set -e

SRC="main.c render/microui.c render/glad.c render/gl1.c render/gl3.c render/overlay.c util/math.c data/cpu.c data/mem.c"
STDFLAGS="-Iinclude -std=c99 -shared -fPIC -Wall"

OUTPUT="libsimpleoverlay.so"
OUTPUT_32="libsimpleoverlay32.so"

WRAPPER="simpleoverlay"

CC="cc"
OS=$(uname)
TARGET="$1"

if [ $OS != "Darwin" ]; then
		SRC="$SRC hooks/dlsym.c hooks/egl.c hooks/glx.c hooks/vulkan.c util/elfhacks.c data/amdgpu.c"
else
		SRC="$SRC hooks/cgl.c data/applegpu.c"
		STDFLAGS="$STDFLAGS -framework Foundation -framework IOKit"
		OUTPUT="libsimpleoverlay.dylib"
fi

if [ $OS = "SunOS" ]; then
		STDFLAGS="$STDFLAGS -lkstat"
fi

if [ ! -e "$(which cc)" ]; then
		if [ -e "$(which gcc)" ]; then
				CC="gcc"
		elif [ -e "$(which clang)" ]; then
				CC="clang"
		else
				echo "No C compiler found!"
				exit
		fi
fi

if [ "$TARGET" = "" ]; then
		if [ $OS != "Darwin" ]; then
			set -x

			$CC $SRC $STDFLAGS -o $OUTPUT
		else
			set -x

			$CC $SRC $STDFLAGS -arch x86_64 -o amd64.dylib
			$CC $SRC $STDFLAGS -arch arm64 -o arm64.dylib
			lipo -create -output $OUTPUT amd64.dylib arm64.dylib
			rm amd64.dylib arm64.dylib
		fi
elif [ "$TARGET" = "multilib" ]; then
		set -x
		
		$CC $SRC -m32 $STDFLAGS -o $OUTPUT_32
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
	
