AC_PREREQ([2.69])
AC_INIT([statusbar], [3.0.0])
AC_CONFIG_SRCDIR()
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for header files.
AC_PATH_X
# AC_CHECK_HEADER(ifaddrs.h, have_sbh="yes", have_sbh="no")
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([sys/statvfs.h])

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create], [], [
		echo "pthread library is required for this program"
		exit -1])
AC_CHECK_LIB([X11], [XStoreName], [], [
		echo "X11 library is required for this program"
		exit -1])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([clock_gettime])
AC_CHECK_FUNCS([localtime_r])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([strncasecmp])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
