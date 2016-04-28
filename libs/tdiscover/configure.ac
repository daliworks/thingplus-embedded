AC_INIT([libtdiscover], [1.0], [dev@dailworks.net])
AM_INIT_AUTOMAKE([foreign -Wall])
AM_PROG_AR
AC_PROG_CC
AC_CONFIG_MACRO_DIR([m4])
LT_INIT
AC_PROG_RANLIB
AC_CHECK_LIB([json-c], [json_c_version_num], [],AC_MSG_ERROR([libjson-c not found]))
AC_CHECK_LIB([curl], [curl_global_init], [],AC_MSG_ERROR([libcurl not found]))
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
