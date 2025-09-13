#!/bin/sh

set -e

SRC_RENDER="src/render/microui.c src/render/glad.c src/render/gl1.c src/render/gl3.c src/render/overlay.c"
SRC_UTIL="src/util/math.c"
SRC_DATA="src/data/cpu.c src/data/mem.c"

SRC_DATA_GPU="src/data/amdgpu.c"
SRC_DATA_GPU_DARWIN="src/data/applegpu.c"

SRC_HOOKS="src/hooks/dlsym.c src/hooks/glx.c src/hooks/egl.c"
SRC_HOOKS_DARWIN="src/hooks/cgl.c"

SRC="src/main.c $SRC_RENDER $SRC_UTIL $SRC_DATA"
STDFLAGS="-Iinclude -std=c99 -shared -fPIC -Wall"

OUTPUT="libsimpleoverlay.so"
OUTPUT_32="libsimpleoverlay32.so"

WRAPPER="simpleoverlay"

CC="cc"
OS=$(uname)
TARGET="$1"

if [ $OS != "Darwin" ]; then
		SRC="$SRC src/util/elfhacks.c $SRC_DATA_GPU $SRC_HOOKS"
else
		SRC="$SRC $SRC_DATA_GPU_DARWIN $SRC_HOOKS_DARWIN"
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
	
