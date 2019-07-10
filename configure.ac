AC_PREREQ([2.69])
AC_INIT([statusbar], [3.0.0])
AM_INIT_AUTOMAKE

# also check for headers in x86_64-linux-gnu
# CFLAGS="$CFLAGS -I/usr/include/x86_64-linux-gnu/"
# CPPFLAGS="$CPPFLAGS -I/usr/include/x86_64-linux-gnu/"

# Check for programs.
AC_PROG_CC

# Check for libraries.
AC_CHECK_LIB([pthread], [pthread_create],
			 [],
			 [AC_MSG_ERROR([pthread library is required for this program])])
AC_CHECK_LIB([m], [powl],
			 [],
			 [AC_MSG_ERROR([math library is required for this program])])
AC_CHECK_LIB([X11], [XStoreName],
			 [],
			 [AC_MSG_ERROR([X11 library is required for this program])])
AC_CHECK_LIB([asound], [snd_mixer_open],
			 [],
			 [volume_lib="no"; AC_MSG_NOTICE([libasound check failed, not running volume routine])])

# Check for standard header files.
AC_PATH_X
AC_CHECK_HEADERS([pthread.h],
				 [],
				 [AC_MSG_ERROR([Missing <pthread.h>])])
AC_CHECK_HEADERS([X11/Xlib.h],
				 [],
				 [AC_MSG_ERROR([Missing <X11/Xlib.h>])])
AC_CHECK_HEADERS([dirent.h],
				 [],
				 [AC_MSG_ERROR([Missing <dirent.h>])])
AC_CHECK_HEADERS([stdlib.h],
				 [],
				 [AC_MSG_ERROR([Missing <stdlib.h>])])
AC_CHECK_HEADERS([unistd.h],
				 [],
				 [AC_MSG_ERROR([Missing <unistd.h>])])
AC_CHECK_HEADERS([stdio.h],
				 [],
				 [AC_MSG_ERROR([Missing <stdio.h>])])
AC_CHECK_HEADERS([string.h],
				 [],
				 [AC_MSG_ERROR([Missing <string.h>])])
AC_CHECK_HEADERS([dirent.h],
				 [],
				 [AC_MSG_ERROR([Missing <dirent.h>])])
AC_CHECK_HEADERS([math.h],
				 [],
				 [AC_MSG_ERROR([Missing <math.h>])])

AC_CHECK_HEADERS([sys/statvfs.h],
				 [disk_headers="yes"],
				 [AC_MSG_NOTICE([Missing <sys/statvfs.h>, not running disk routine])])
AC_CHECK_HEADERS([ctype.h],
				 [todo_headers="yes"],
				 [AC_MSG_NOTICE([Missing <ctype.h>, not running TODO routine])])
AC_CHECK_HEADERS([sys/ioctl.h ifaddrs.h net/if.h],
				 [net_headers="yes"],
				 [net_headers="no";
				  AC_MSG_NOTICE([Missing a network header, not running network or wifi routines]);
				  break])
AC_CHECK_HEADERS([linux/wireless.h],
				 [wifi_headers="yes"],
				 [AC_MSG_NOTICE([Missing <linux/wireless.h>, not running wifi routine])])
AC_CHECK_HEADERS([alsa/asoundlib.h],
				 [volume_headers="yes"],
				 [AC_MSG_NOTICE([Missing <alsa/asoundlib.h>, not running volume routine])])

# Check for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Check for library functions.
AC_CHECK_FUNCS([clock_gettime], [], [AC_MSG_ERROR([Unable to run `clock_gettime`])])
AC_CHECK_FUNCS([localtime_r],   [], [AC_MSG_ERROR([Unable to run `localtime_r`])])
AC_CHECK_FUNCS([memset],        [], [AC_MSG_ERROR([Unable to run `memset`])])
AC_CHECK_FUNCS([socket],        [], [AC_MSG_ERROR([Unable to run `socket`])])
AC_CHECK_FUNCS([strncasecmp],   [], [AC_MSG_ERROR([Unable to run `strncasecmp`])])

AS_IF([test "$volume_headers" = "yes" -a "$volume_lib" != "no"], [AC_DEFINE([BUILD_VOLUME])])
AS_IF([test "$disk_headers"   = "yes"], [AC_DEFINE([BUILD_DISK])])
AS_IF([test "$todo_headers"   = "yes"], [AC_DEFINE([BUILD_TODO])])
AS_IF([test "$net_headers"    = "yes"], [AC_DEFINE([BUILD_NETWORK])])
AS_IF([test "$net_headers"    = "yes" -a "$wifi_headers" = "yes"], AC_DEFINE([BUILD_WIFI])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
