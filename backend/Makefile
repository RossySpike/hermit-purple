.PHONY: test
CC := gcc
DEBUG_MAX_CYCLES ?= 10
CFLAGS_BASE := -I$(shell pwd)/includes -std=gnu23 -Wno-unused-parameter -Wpedantic -Wall -Wextra \
                -fstack-protector-strong -fcf-protection
CFLAGS_PROD := $(CFLAGS_BASE) -fhardened

# LD = Linker directory
BUNDLED_LD_FLAGS := $(LD_BASE) -L$(shell pwd)/includes/libs/vips/lib  $(shell pkg-config --cflags --libs glib-2.0 gobject-2.0)  -lvips

LD_FLAGS := $(LD_BASE)  $(shell pkg-config --cflags --libs glib-2.0 gobject-2.0 vips)
SRCS     := $(wildcard ./src/*.c) main.c


make:
	$(CC) $(LD_FLAGS) -O2  $(CFLAGS_PROD) $(SRCS) -o ./test/hermit-purple
make-bundled: 
	$(CC) -DIS_BUNDLED $(BUNDLED_LD_FLAGS) -O2  $(CFLAGS_PROD) $(SRCS) -o ./test/hermit-purple.bundled

debug:
	$(CC) $(LD_FLAGS) -g -O0 $(CFLAGS_BASE) $(SRCS) -o ./test/main.debug.out
debug-bundled:
	$(CC) $(BUNDLED_LD_FLAGS) -g -O0 $(CFLAGS_BASE) $(SRCS) -o ./test/main.bundled.debug.out
run-debug:
	make debug
	./test/main.debug.out

test:
		$(CC) -DDEBUG_MAX_CYCLES=$(DEBUG_MAX_CYCLES) $(LD_FLAGS) -g -O0 $(CFLAGS_BASE) $(SRCS) -o ./test/main.test.out

run-test:
	make test
	valgrind \
  --leak-check=full \
  --show-leak-kinds=definite,indirect \
  --track-origins=yes \
  --suppressions=/usr/share/glib-2.0/valgrind/glib.supp \
  ./test/main.test.out
