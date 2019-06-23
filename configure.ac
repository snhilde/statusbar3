AC_PREREQ([2.69])
AC_INIT([statusbar], [3.0.0])
AM_INIT_AUTOMAKE

# Check for programs.
AC_PROG_CC

# Check for libraries.
AC_CHECK_LIB([pthread], [pthread_create], [], [
		echo "pthread library is required for this program"
		exit -1])
AC_CHECK_LIB([X11], [XStoreName], [], [
		echo "X11 library is required for this program"
		exit -1])

# Check for header files.
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

# headers for disk routine
AC_CHECK_HEADERS([sys/statvfs.h],
				 [AC_DEFINE([BUILD_DISK], 1)],
				 [AC_DEFINE([BUILD_DISK], 0),
				  AC_MSG_NOTICE([Missing <sys/statvfs.h>, not building support for disk usage])])
# headers for todo routine
AC_CHECK_HEADERS([ctype.h],
				 [AC_DEFINE([BUILD_TODO], 1)],
				 [AC_DEFINE([BUILD_TODO], 0),
				  AC_MSG_NOTICE([Missing <ctype.h>, not building support for reading TODO file])])
# headers for wifi routine
AC_CHECK_HEADERS([sys/ioctl.h linux/wireless.h ifaddrs.h],
				 [AC_DEFINE([BUILD_WIFI], 1)],
				 [AC_DEFINE([BUILD_WIFI], 0),
				  AC_MSG_NOTICE([Not building wireless support]); break])

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
