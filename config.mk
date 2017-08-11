NAME    = wd
VERSION = 1.0

# Customize below to fit your system

# paths
PREFIX = /tmp
MANPREFIX = ${PREFIX}/share/man

# includes and libs
INCS = -I. -I/usr/include

# flags
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_DEFAULT_SOURCE
CFLAGS = -g -std=c99 -pedantic -Wall -O2 ${INCS} ${CPPFLAGS}
#CFLAGS = -std=c99 -pedantic -Wall -Os ${INCS} ${CPPFLAGS}
LDFLAGS = -g ${LIBS}
#LDFLAGS = -s ${LIBS}

# Solaris
#CFLAGS = -fast ${INCS} -DVERSION=\"${VERSION}\"
#LDFLAGS = ${LIBS}


# compiler and linker
CC = cc

