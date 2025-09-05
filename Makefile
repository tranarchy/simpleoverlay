SRC = main.c microui.c glad.c hooks/dlsym.c hooks/egl.c hooks/glx.c render/render.c render/overlay.c util/elfhacks.c util/amdgpu.c util/cpu.c util/mem.c
SRC_MACOS = main.c microui.c glad.c hooks/cgl.c render/render.c render/overlay.c util/amdgpu.c util/cpu.c util/mem.c

STDFLAGS = -shared -fPIC -Wall

OUTPUT = libsimpleoverlay.so
OUTPUT_32 = libsimpleoverlay32.so

OUTPUT_MACOS = libsimpleoverlay.dylib

SCRIPT = simpleoverlay

main:
	cc $(SRC) $(STDFLAGS) -o $(OUTPUT)

macos:
	cc $(SRC_MACOS) $(STDFLAGS) -framework Foundation -o $(OUTPUT_MACOS)

multilib: main
	cc $(SRC) -m32 $(STDFLAGS) -o $(OUTPUT_32)

install:
	mkdir -p /usr/local/bin
	chmod +x $(SCRIPT)
	cp -f $(SCRIPT) /usr/local/bin

	mkdir -p /usr/local/lib
	cp -f libsimpleoverlay.* /usr/local/lib

install_multilib: install
	cp -f $(OUTPUT_32) /usr/lib32/$(OUTPUT)
