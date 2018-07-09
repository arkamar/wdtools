NAME    = wd
VERSION = 1.0

# Customize below to fit your system

# paths
PREFIX = /tmp
MANPREFIX = ${PREFIX}/share/man

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_DEFAULT_SOURCE -DWD_COLORS
CFLAGS ?= -O2
CFLAGS += -std=c99 -pedantic -Wall
CFLAGS += -fPIC
