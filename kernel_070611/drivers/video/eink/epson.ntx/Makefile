#
# Makefile for the Epson s1d13521 framebuffer device driver
#

obj-$(CONFIG_FB_EPSON_S1D13521)	+= s1d13521fb.o
s1d13521fb-objs	+= s1d13521base.o access.o PVI_conv.o move.o
s1d13521fb-$(CONFIG_FB_EPSON_PROC) += s1d13521proc.o
EXTRA_CFLAGS += -D'S1D13xxxFB_INCLUDE="s1d13521.h"'

