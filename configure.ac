AC_INIT([libwrap], [1.0])
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AR

AC_ARG_ENABLE([32], AS_HELP_STRING([--enable-32], [Force 32bit arch]), ARCH=-m32 SUFFIX=_32)
AC_ARG_ENABLE([custom-log-path],
			  AS_HELP_STRING([--enable-custom-log-path], [Set the path where the logs should be saved]),
			  [ if test "x$enableval" = "xno" ; then
				  LOG_PATH=./
			   elif test "x$enableval" = "xyes" ; then
				  LOG_PATH=./
			   else
				  LOG_PATH=$enableval
			   fi
			 ],
			 [
			 	LOG_PATH=./
			 ])

AS_IF(
[test "$host" = "arm-linux-gnueabi"],
	[OPTS="-g -O0 -rdynamic -fno-omit-frame-pointer -mapcs-frame -funwind-tables -DLOG_PATH='\"$LOG_PATH\"'" SUFFIX=_arm],
	[OPTS="-g -O0 -rdynamic -fno-omit-frame-pointer $ARCH -DLOG_PATH='\"$LOG_PATH\"'"]
)

AC_SUBST(ARCH)
AC_SUBST(OPTS)
AC_SUBST(SUFFIX)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
