libvip
libheif
sudo pacman -S libjpeg-turbo
sudo pacman -S libheif
sudo pacman -S libpng
sudo pacman -S libwebp

```bash
meson setup build --buildtype=release   -Dintrospection=disabled   -Dcplusplus=false   -Ddeprecated=false   -Dexamples=false   -Dpoppler=disabled   -Dpdfium=disabled   -Dmagick=disabled   -Dtiff=disabled   -Dexif=disabled   -Dcgif=disabled   -Dnsgif=false   -Dppm=false   --prefix=/home/honey/proyectos/hermit-purple/backend/includes/libs/vips

meson compile -C build
```
