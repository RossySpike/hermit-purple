.PHONY: test
CC := gcc
DEBUG_MAX_CYCLES ?= 100
PKG_DEPS := glib-2.0 gobject-2.0
CFLAGS_BASE := -I$(shell pwd)/includes -std=gnu23 -Wno-unused-parameter -Wpedantic -Wall -Wextra \
                -fstack-protector-strong -fcf-protection  $(shell pkg-config --cflags $(PKG_DEPS))
CFLAGS_PROD := $(CFLAGS_BASE) -fhardened

# LD = Linker directory
LD_BASE := $(shell pkg-config --libs $(PKG_DEPS))
BUNDLED_LD_FLAGS := $(LD_BASE) -L$(shell pwd)/includes/libs/vips/lib   -lvips

LD_FLAGS := $(LD_BASE)  $(shell pkg-config --libs  vips)
SRCS     := $(wildcard ./src/*.c) main.c


make:
	$(CC)  -O2   $(CFLAGS_PROD) $(shell pkg-config --cflags vips)  $(SRCS) -o ./test/hermit-purple $(LD_FLAGS)
make-bundled: 
	$(CC) -DIS_BUNDLED  -O2  $(CFLAGS_PROD) $(SRCS) -o ./test/hermit-purple.bundled $(BUNDLED_LD_FLAGS)

debug:
	$(CC)  -DDEBUG_MAX_CYCLES=$(DEBUG_MAX_CYCLES) -g -O0 -fsanitize=address -fno-omit-frame-pointer $(CFLAGS_BASE) $(SRCS) -o ./test/main.debug.out $(LD_FLAGS) 
debug-bundled:
	$(CC)  -DIS_BUNDLED -g -O0  $(CFLAGS_BASE) $(SRCS) -o ./test/main.bundled.debug.out $(BUNDLED_LD_FLAGS)
run-debug: debug
	./test/main.debug.out

test:
		$(CC)  -g -O0 $(CFLAGS_BASE) $(SRCS) -o ./test/main.test.out $(LD_FLAGS)

run-valgrind: test
	valgrind \
	-s \
  --leak-check=full \
  --show-leak-kinds=definite,indirect \
  --track-origins=yes \
  --suppressions=/usr/share/glib-2.0/valgrind/glib.supp \
  ./test/main.test.out

run-tests-dev: test
	./test/main.test.out & PID=$$! ; \
	sleep 2 ; \
	cd ./tests/ && ./run_tests.sh ; \
	TEST_STATUS=$$?;\
	kill $$PID;
	echo "TEST_STATUS: $$TEST_STATUS" \
	exit $$TEST_STATUS

run-tests-prod: make
	./test/hermit-purple & PID=$$! ; \
	sleep 2 ; \
	cd ./tests/ && ./run_tests.sh ; \
	TEST_STATUS=$$?;\
	kill $$PID;
	echo "TEST_STATUS: $$TEST_STATUS" \
	exit $$TEST_STATUS


clean:
	rm -f ./.gdb_history
	rm -f ./*.log
	rm -f ./vgcore.*
	rm -f ./test/*
	rm -f ./tests/endpoints/api/image/get/*.log
	rm -f ./tests/endpoints/api/image/get/test.*.*
	rm -f ./tests/endpoints/api/image/post/*.log

