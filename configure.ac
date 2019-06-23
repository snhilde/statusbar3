AC_PREREQ([2.69])
AC_INIT([statusbar], [3.0.0])
AM_INIT_AUTOMAKE

# also check for headers in x86_64-linux-gnu
# CFLAGS="$CFLAGS -I/usr/include/x86_64-linux-gnu/"
# CPPFLAGS="$CPPFLAGS -I/usr/include/x86_64-linux-gnu/"

# Check for programs.
AC_PROG_CC

# Check for libraries.
AC_CHECK_LIB([pthread], [pthread_create], [], [
		echo "pthread library is required for this program"
		exit -1])
AC_CHECK_LIB([X11], [XStoreName], [], [
		echo "X11 library is required for this program"
		exit -1])

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

AC_CHECK_HEADERS([sys/statvfs.h],
				 [build_disk="yes"],
				 [AC_MSG_NOTICE([Missing <sys/statvfs.h>, not running disk routine])])
AC_CHECK_HEADERS([ctype.h],
				 [build_todo="yes"],
				 [AC_MSG_NOTICE([Missing <ctype.h>, not running TODO routine])])
AC_CHECK_HEADERS([linux/wireless.h sys/ioctl.h ifaddrs.h],
				 [build_network="yes"],
				 [build_network="no"; AC_MSG_NOTICE([Missing a header, not running network of wifi routines]); break])
AC_CHECK_HEADERS([alsa/mixer.h],
				 [build_volume="yes"],
				 [AC_MSG_NOTICE([Missing <alsa/mixer.h>, not running volume routine])])

AS_IF([test "$build_volume"  = "yes"], [AC_DEFINE([BUILD_VOLUME])])
AS_IF([test "$build_disk"    = "yes"], [AC_DEFINE([BUILD_DISK])])
AS_IF([test "$build_todo"    = "yes"], [AC_DEFINE([BUILD_TODO])])
AS_IF([test "$build_network" = "yes"], [AC_DEFINE([BUILD_NETWORK]) AC_DEFINE([BUILD_WIFI])])

# Check for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Check for library functions.
AC_CHECK_FUNCS([clock_gettime])
AC_CHECK_FUNCS([localtime_r])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([strncasecmp])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
