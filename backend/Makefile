CC := gcc
CFLAGS_BASE := -std=gnu23 -Wno-unused-parameter -Wpedantic -Wall -Wextra \
               -fhardened -fstack-protector-strong -fcf-protection
LD_FLAGS := -L $(shell pkg-config --cflags --libs glib-2.0 gobject-2.0)  -lvips
SRCS     := $(wildcard ./src/*.c) main.c


make: 
	$(CC) $(LD_FLAGS) -O2 $(CFLAGS_BASE) $(SRCS) -o ./test/hermit-purple

debug:
	$(CC) $(LD_FLAGS) -O0 $(CFLAGS_BASE) $(SRCS) -o ./test/main.test.out
