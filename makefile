ARCH ?= 32
CFLAGS = -O3 -m$(ARCH)
srcfiles := $(shell find . -name "*.c")
incfiles := $(shell find . -name "*.h")

j1: $(srcfiles) $(incfiles)
	$(CC) $(CFLAGS) $(srcfiles) -o $@

run: j1
	./j1

clean:
	rm -f j1

test: j1
	./j1 -f:j1 -c:10000

bin: j1
	cp -u -p j1 ~/bin/
