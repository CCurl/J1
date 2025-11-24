ARCH ?= 32
CFLAGS = -O3 -m$(ARCH)
srcfiles := $(shell find . -name "*.c")
incfiles := $(shell find . -name "*.h")

j1-emu: j1-emu.c j1.h
	$(CC) $(CFLAGS) j1-emu.c -o $@

j1-parse: j1-parse.c j1-dis.c j1.h
	$(CC) $(CFLAGS) j1-parse.c j1-dis.c -o $@

run: j1-emu
	./j1-emu

clean:
	rm -f j1-emu
	rm -f j1-parse

test: j1-parse j1-emu
	./j1-parse
	./j1-emu

bin: j1-emu
	cp -u -p j1-emu ~/bin/
