SRC = main.c glad.c util/elfhacks.c util/amdgpu.c util/cpu.c util/mem.c
STDFLAGS = -shared -fPIC -Wall

OUTPUT = libsimpleoverlay.so
OUTPUT_32 = libsimpleoverlay32.so
SCRIPT = simpleoverlay

main:
	cc $(SRC) $(STDFLAGS) -o $(OUTPUT)

multilib: main
	cc $(SRC) -m32 $(STDFLAGS) -o $(OUTPUT_32)

install:
	chmod +x $(SCRIPT)
	cp -f $(SCRIPT) /usr/bin

	cp -f $(OUTPUT) /usr/lib

install_multilib: install
	cp -f $(OUTPUT_32) /usr/lib32/$(OUTPUT)
