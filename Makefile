VERSION= 3.0

CC= gcc

INCDIR=     /usr/include
X11_INCDIR= /usr/include/X11

LIBDIR=     /usr/lib/x86_64-linux-gnu
X11_LIBDIR= /usr/lib/X11

CFLAGS= -I ${INCDIR} \
	    -I ${X11_INCDIR}

LDFLAGS= -L ${LIBDIR} \
	     -L ${X11_LIBDIR} \
	     -l pthread \
         -l X11

statusbar: statusbar.h config.h
	${CC} -g -O0 -o $@ statusbar.c ${LDFLAGS}
