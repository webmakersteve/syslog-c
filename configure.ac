AC_PREREQ([2.69])
AC_INIT([syslog.c], [0.1], [webmakersteve@gmail.com])
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_PROG_RANLIB

AC_CONFIG_SRCDIR([src/])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdlib.h string.h unistd.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([mkdir strchr strdup strerror])

# Run python to generate tests
python tests/generate.py tests

AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile bench/Makefile])
AC_OUTPUT
