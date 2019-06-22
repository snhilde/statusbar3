VERSION=3.0

CC=gcc

INCDIR=/usr/include
LIBDIR=/usr/lib/x86_64-linux-gnu 

X11INCDIR=/usr/include/X11
X11LIBDIR=/usr/lib/X11

INCS= -I ${INCDIR} \
	  -I ${X11INCDIR}

LIBS= -L ${LIBDIR} \
	  -L ${X11LIBDIR} \
	  -l pthread \
      -l X11

statusbar: statusbar.c
	${CC} -g -O0 -o statusbar statusbar.c ${INCS} ${LIBS}
