#
# cz-2017:  never made a config file to generate these locations:
#
top_srcdir   = /usr/home/curt/source/php-4.0.1pl2
top_builddir = /usr/home/curt/source/php-4.0.1pl2
srcdir       = /usr/home/curt/source/php-4.0.1pl2/ext/frotz
builddir     = /usr/home/curt/source/php-4.0.1pl2/ext/frotz
VPATH        = /usr/home/curt/source/php-4.0.1pl2/ext/frotz

LTLIBRARY_NAME		= libfrotz.la
LTLIBRARY_SOURCES	= php_frotz.c ux_init.c ux_input.c  ux_pic.c ux_screen.c ux_sample.c ux_text.c save.c
LTLIBRARY_DEPENDENCIES	= Z/libZ.la
LTLIBRARY_LIBADD	= $(LTLIBRARY_DEPENDENCIES)
LTLIBRARY_SHARED_NAME	= frotz.la
LTLIBRARY_SHARED_LIBADD	= $(LTLIBRARY_LIBADD)

SUBDIRS = Z

include $(top_srcdir)/build/dynlib.mk
