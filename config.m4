dnl $Id$
dnl config.m4 for extension pwee

PHP_ARG_WITH(pwee, for pwee support,
[  --with-pwee             Enable environment extensions])

if test "$PHP_PWEE" != "no"; then
  AC_CHECK_LIB(uuid, uuid_generate,
	[AC_DEFINE(HAVE_LIBUUID,1,[ ]) 
		PHP_ADD_LIBRARY(uuid, 1, PWEE_SHARED_LIBADD)],
	[AC_MSG_WARN(using internal version of uuid_generate)])

  AC_MSG_CHECKING([for ioctl])
  AC_TRY_LINK_FUNC(ioctl,
	[AC_DEFINE(HAVE_IOCTL,1,[ ])
		AC_MSG_RESULT([yes])],
	[AC_MSG_ERROR(this version requires ioctl)])

  PHP_EXTENSION(pwee, $ext_shared)
  PHP_SUBST(PWEE_SHARED_LIBADD)
fi
