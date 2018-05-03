#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([devicehive-datalogger], [0.1], [casper.mogensen@gmail.com])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE
# Checks for programs.
AC_PROG_CC

LT_INIT

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
common_CFLAGS="\
	-I../src \
	-I../src/devicehive-logger/src \
	-Wall"
AC_SUBST([common_CFLAGS])
common_LDFLAGS="-lrt -lsqlite3 -lcurl -lm -lpthread"
AC_SUBST([common_LDFLAGS])

AC_CONFIG_SUBDIRS([src/devicehive-logger])
AC_CONFIG_FILES([Makefile  src/Makefile])
AC_OUTPUT
