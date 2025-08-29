SRC = main.c elfhacks.c amdgpu.c cpu.c mem.c
STDFLAGS = -shared -fPIC
STDFLAGS += $(shell pkg-config --libs --cflags libdrm,libdrm_amdgpu,gl,egl)
STDFLAGS += $(shell pkg-config --libs --cflags glx)
OUTPUT = libsimpleoverlay.so
SCRIPT = simpleoverlay

main:
	cc $(SRC) $(STDFLAGS) -o $(OUTPUT)

install:
	cp -f $(OUTPUT) /usr/lib

	chmod +x $(SCRIPT)
	cp -f $(SCRIPT) /usr/bin
