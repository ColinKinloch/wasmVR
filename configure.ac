AC_INIT([wasmVR],
        [0.0.0],
        [colin@kinlo.ch],
        [wasmVR])

AC_CONFIG_SRCDIR([src/wasmVR.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AC_PROG_CXX
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11

AC_SUBST(total_memory, 50331648)

AM_INIT_AUTOMAKE([foreign -Wall -Werror -Wno-portability subdir-objects])

#AC_CHECK_HEADERS([glm/glm.hpp], [],
#  [AC_MSG_FAILURE([glm header missing])])

AC_PREFIX_DEFAULT([$(pwd)/system])

AC_CONFIG_FILES([
  Makefile
  index.html
])
AC_OUTPUT
