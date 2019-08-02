AUTOMAKE_OPTIONS=foreign

CFLAGS= -g -O0 -Wall \
		-fsanitize=leak \
		-fno-omit-frame-pointer \
        -DVERSION=\"$(VERSION)\" \
        -DPROG="\"$(PACKAGE)\"" \
        -I /usr/include \
        -I /usr/include/X11

LDFLAGS= -L /usr/lib/x86_64-linux-gnu \
         -L /usr/local/lib \
         -L /usr/lib/X11

bin_PROGRAMS= statusbar
statusbar_SOURCES= statusbar.c cJSON.c
