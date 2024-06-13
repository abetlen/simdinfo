TEST_SIZE ?= 4096

all: test-x86_64 test-static-x86_64 test-aarch64

bin:
	mkdir -p bin

bin/main-x86_64: main.c vdot.h simdinfo.h bin
	gcc \
		-o bin/main-x86_64 \
		main.c \
		-march=x86-64 \
		-mavx -mavx2 -mf16c -mfma -msse4.1 -msse4.2 -mavx512f \
		-mtune=generic \
		-I. \
		-O2 \
		-Wall

test-x86_64: bin/main-x86_64
	./bin/main-x86_64 $(TEST_SIZE)

bin/main-static-x86_64: main.c vdot.h simdinfo.h bin
	gcc \
		-o bin/main-static-x86_64 \
		main.c \
		-march=native \
		-I. \
		-O3 \
		-Wall \
		-DVDOT_STATIC_DISPATCH

test-static-x86_64: bin/main-static-x86_64
	./bin/main-static-x86_64 $(TEST_SIZE)

# compiling on gcc < 11.1 will result in missing arm_sve.h
# clang is used here instead
bin/main-aarch64: main.c vdot.h simdinfo.h bin
	clang-16 \
		-target aarch64-linux-gnu \
		-o bin/main-aarch64 \
		main.c \
		-march=armv8-a+sve \
		-mtune=generic \
		-I. \
		-O2 \
		-Wall

test-aarch64: bin/main-aarch64
	qemu-aarch64 \
		-cpu max,sve=on \
		-L /usr/aarch64-linux-gnu \
		./bin/main-aarch64 $(TEST_SIZE)

clean:
	rm -f bin/*

.PHONY: test clean