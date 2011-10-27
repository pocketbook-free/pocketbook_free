/* Use libjpeg instead of builtin jpeg decoder. */
#define ENABLE_LIBJPEG 1

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.*/
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Have FreeType2 include files */
#define HAVE_FREETYPE_H 1

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#define HAVE_FSEEKO 1

/* Defines if gettimeofday is available on your system */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Defines if localtime_r is available on your system */
#define HAVE_LOCALTIME_R 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkstemp' function. */
#define HAVE_MKSTEMP 1

/* Define to 1 if you have the `popen' function. */
#define HAVE_POPEN 1

/* Define if you have POSIX threads libraries and header files. */
#define HAVE_PTHREAD 1

/* Use splash for rendering. */
#define HAVE_SPLASH 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Enable multithreading support. */
#define MULTITHREADED 1

/* Generate OPI comments in PS output. */
#define OPI_SUPPORT 1

/* Name of package */
#define PACKAGE "poppler"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "poppler"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "poppler 0.8.4"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "poppler"

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.8.4"

/* Poppler data dir */
#define POPPLER_DATADIR "/home/netspider/projects/FRSCSDK/arm-none-linux-gnueabi/sysroot/usr/share/poppler"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Enable word list support. */
#define TEXTOUT_WORD_LIST 1

/* Use fixed point arithmetic */
#define USE_FIXEDPOINT 1

/* Version number of package */
#define VERSION "0.8.4"

/* Define to 1 if the X Window System is missing or not being used. */
#define X_DISPLAY_MISSING 1

/*
 * jpeg.h needs HAVE_BOOLEAN, when the system uses boolean in system
 * headers and I'm too lazy to write a configure test as long as only
 * unixware is related
 */
#ifdef _UNIXWARE
#define HAVE_BOOLEAN
#endif


/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

