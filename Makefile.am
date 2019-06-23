AUTOMAKE_OPTIONS=foreign

INCDIR=     /usr/include
X11_INCDIR= /usr/include/X11

LIBDIR=     /usr/lib/x86_64-linux-gnu
X11_LIBDIR= /usr/lib/X11

CFLAGS= -Wall \
        -DVERSION=\"$(VERSION)\" \
        -DPROG="\"$(PACKAGE)\"" \
        -I ${INCDIR} \
	    -I ${X11_INCDIR}

LDFLAGS= -L ${LIBDIR} \
	     -L ${X11_LIBDIR} \
	     -l pthread \
         -l X11 \
		 -l asound

bin_PROGRAMS= statusbar
