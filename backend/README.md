# HERMIT PURPLE BACKEND

## INSTALLING DEPS

### via package manager

- libvip
- libheif
- libjpeg-turbo
- libheif
- libpng
- libwebp

```bash
meson setup build --buildtype=release   -Dintrospection=disabled   -Dcplusplus=false   -Ddeprecated=false   -Dexamples=false   -Dpoppler=disabled   -Dpdfium=disabled   -Dmagick=disabled   -Dtiff=disabled   -Dexif=disabled   -Dcgif=disabled   -Dnsgif=false   -Dppm=false   --prefix="$ROUTE_TO_VIPS_DIR"

meson compile -C build
```

## TESTING

First make sure you have a proper `tests.env` file.

```bash
cat << EOF > ./tests/tests.env
PATH_TO_TESTS_DIR="ABSOLUTE/PATH/TO/TESTS/DIR"
HOST="localhost:1600"
IMG_ORIGINAL_DIR="/tmp/original/"
IMG_THUMBNAIL_DIR="/tmp/thumbnail/"
IMG_CACHE_DIR="/tmp/cache/"
EOF
```

Then you can run:

```bash
make runt-tests
```
