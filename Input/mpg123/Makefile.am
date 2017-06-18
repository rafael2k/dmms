lib_LTLIBRARIES = libmpg123.la

libdir = $(plugindir)/$(INPUT_PLUGIN_DIR)


COMMON_SRC = \
mpg123.c mpg123.h \
common.c \
tabinit.c l2tables.h \
configure.c fileinfo.c \
decode_2to1.c decode_4to1.c \
layer1.c layer2.c layer3.c \
equalizer.c http.c \
huffman.h getbits.h \
dxhead.c dxhead.h \
id3.c id3.h \
id3_frame.c id3_frame_content.c id3_frame_text.c id3_frame_url.c \
id3_header.h id3_tag.c unicode.c

if ARCH_X86

if USE_X86ASM

if USE_SIMD

EXTRA_SRC = decode_i386.c dct64_i386.c decode_i586.s \
            decode_3dnow.s dct64_3dnow.s dct36_3dnow.s getcpuflags.s \
            dct64_MMX.s decode_MMX.s

else

EXTRA_SRC = decode_i386.c dct64_i386.c decode_i586.s 

endif

else

EXTRA_SRC = decode_i386.c dct64_i386.c

endif

else

EXTRA_SRC = decode.c dct64.c

endif

EXTRA_DIST = decode.c decode_i386.c decode_i586.s decode_3dnow.s dct64_i386.c \
             getbits.c dct64.c dct64_3dnow.s dct36_3dnow.s getcpuflags.s \
             dct64_MMX.s decode_MMX.s

libmpg123_la_SOURCES = $(COMMON_SRC) $(EXTRA_SRC)
libmpg123_la_LDFLAGS = @PLUGIN_LDFLAGS@
libmpg123_la_LIBADD = @GTK_LIBS@ @PTHREAD_LIBS@ @Z_LIBS@ $(top_builddir)/libxmms/libxmms.la
INCLUDES = @GTK_CFLAGS@ @ARCH_DEFINES@ -I$(top_builddir)/intl -I$(top_srcdir)
