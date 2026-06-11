/* Macros for the header version.
 */

#ifndef VIPS_VERSION_H
#define VIPS_VERSION_H

#define VIPS_VERSION "8.18.3"
#define VIPS_VERSION_STRING "8.18.3"
#define VIPS_MAJOR_VERSION (8)
#define VIPS_MINOR_VERSION (18)
#define VIPS_MICRO_VERSION (3)

/* The ABI version, as used for library versioning.
 */
#define VIPS_LIBRARY_CURRENT (62)
#define VIPS_LIBRARY_REVISION (3)
#define VIPS_LIBRARY_AGE (20)

#define VIPS_CONFIG "enable debug: false\nenable deprecated: false\nenable modules: true\nenable C++ binding: false\nenable RAD load/save: true\nenable Analyze7 load: true\nenable PPM load/save: false\nenable GIF load: false\nFFTs with fftw3: true\nSIMD support with libhwy: true\nICC profile support with lcms2: true\ndeflate compression with zlib: true\ntext rendering with pangocairo: true\nfont file support with fontconfig: true\nEXIF metadata support with libexif: false\nJPEG load/save with libjpeg: true\nUHDR load/save with libuhdr: false\nJXL load/save with libjxl: true (dynamic module: true)\nJPEG2000 load/save with libopenjp2: true\nPNG load/save with libpng: true\nimage quantisation with imagequant: true\nTIFF load/save with libtiff: false\nimage pyramid save with libarchive: true\nHEIC/AVIF load/save with libheif: true (dynamic module: true)\nWebP load/save with libwebp: true\nPDF load with PDFium or Poppler: false (dynamic module: false)\nSVG load with librsvg-2.0: true\nEXR load with OpenEXR: true\nWSI load with OpenSlide: false (dynamic module: false)\nMatlab load with Matio: false\nNIfTI load/save with libnifti: false\nFITS load/save with cfitsio: true\nGIF save with cgif: false\nRAW load with libraw_r: true\nMagick load/save with MagickCore: false (dynamic module: false)"

/* Not really anything to do with versions, but this is a handy place to put
 * it.
 */
#define VIPS_ENABLE_DEPRECATED 0

#endif /*VIPS_VERSION_H*/
