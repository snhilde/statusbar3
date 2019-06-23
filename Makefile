VERSION= 3.0
CC= gcc
DESTDIR= /usr/local/bin

PROGNAME= statusbar

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

${PROGNAME}: statusbar.h config.h
	${CC} -g -O0 -o $@ statusbar.c ${LDFLAGS}

.PHONY: clean
clean:
	rm ${PROGNAME}

.PHONY: install
install:
	mkdir -p ${DESTDIR}
	cp ${PROGNAME} ${DESTDIR}/${PROGNAME}

.PHONY: uninstall
uninstall:
	rm -f ${DESTDIR}/${PROGNAME}
