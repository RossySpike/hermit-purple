.PHONY: test
make:
	# gcc -march=znver3 -O2 main.c -o ./test/main.out
	 gcc -O2 -I/home/honey/proyectos/hermit-purple/backend/includes/libs/vips/include     -L/home/honey/proyectos/hermit-purple/backend/includes/libs/vips/lib     `pkg-config --cflags --libs glib-2.0 gobject-2.0`     -lvips  -Wno-unused-parameter -Wpedantic -Wall -Wextra     -fhardened -fstack-protector-strong -fcf-protection -std=c23     ./src/api-image.c ./src/common-response.c ./src/defines.c ./src/files.c ./src/http-sanitize.c ./src/server-defines.c ./src/list.c ./src/server-machine.c ./src/server-routes.c ./src/server.c main.c -o ./test/main.out
debug:
	 gcc -O0 fsanitize=address,undefined,leak -g -I/home/honey/proyectos/hermit-purple/backend/includes/libs/vips/include     -L/home/honey/proyectos/hermit-purple/backend/includes/libs/vips/lib     `pkg-config --cflags --libs glib-2.0 gobject-2.0`     -lvips  -Wno-unused-parameter -Wpedantic -Wall -Wextra     -fhardened -fstack-protector-strong -fcf-protection -std=c23     ./src/api-image.c ./src/common-response.c ./src/defines.c ./src/files.c ./src/http-sanitize.c ./src/server-defines.c ./src/list.c ./src/server-machine.c ./src/server-routes.c ./src/server.c main.c -o ./test/main.debug.out
test:
	# gcc -I/home/honey/proyectos/hermit-purple/backend/includes/libs/vips/include   -L/home/honey/proyectos/hermit-purple/backend/includes/libs/vips/lib   `pkg-config --cflags --libs glib-2.0 gobject-2.0`   -lvips  -g -O0 -Wno-unused-parameter -Wpedantic -Wall -Wextra -fhardened -fstack-protector-strong -fcf-protection -Os -std=c23  main.c -o ./test/main.test.out
	 gcc -I/home/honey/proyectos/hermit-purple/backend/includes/libs/vips/include     -L/home/honey/proyectos/hermit-purple/backend/includes/libs/vips/lib     `pkg-config --cflags --libs glib-2.0 gobject-2.0`     -lvips -g -O0 -Wno-unused-parameter -Wpedantic -Wall -Wextra     -fhardened -fstack-protector-strong -fcf-protection -std=c23     ./src/api-image.c ./src/common-response.c ./src/defines.c ./src/files.c ./src/http-sanitize.c ./src/server-defines.c ./src/list.c ./src/server-machine.c ./src/server-routes.c ./src/server.c main.c -o ./test/main.test.out
# perf:
# 	gcc  -O3  -march=znver3 -std=c23  main.c -o ./test/main.perf.out

static:
	gcc -O2 -I/home/honey/proyectos/hermit-purple/backend/includes/libs/vips/include \
    -L/home/honey/proyectos/hermit-purple/backend/includes/libs/vips/lib \
    -Wl,-Bstatic -lc -lgcc -lgcc_eh -Wl,-Bdynamic \
    `pkg-config --cflags --libs glib-2.0 gobject-2.0` \
    -lvips -Wno-unused-parameter -Wpedantic -Wall -Wextra \
    -fhardened -fstack-protector-strong -fcf-protection -std=c23 \
    ./src/api-image.c ./src/common-response.c ./src/defines.c ./src/files.c \
    ./src/http-sanitize.c ./src/server-defines.c ./src/list.c \
    ./src/server-machine.c ./src/server-routes.c ./src/server.c \
    main.c -o ./test/main.static.out
