dnl $Id$
dnl config.m4 for extension pwee

AC_DEFUN([PWEE_XML_CHECK_VERSION],[
  old_CPPFLAGS=$CPPFLAGS
  CPPFLAGS=-I$PWEE_XML_DIR/include$PWEE_XML_DIR_ADD
  AC_MSG_CHECKING(for libxml version)
  AC_EGREP_CPP(yes,[
#include <libxml/xmlversion.h>
#if LIBXML_VERSION >= 20414
  yes
#endif
  ],[
    AC_MSG_RESULT(>= 2.4.14)
  ],[
    AC_MSG_ERROR(libxml version 2.4.14 or greater required.)
  ])
  CPPFLAGS=$old_CPPFLAGS
])

PHP_ARG_WITH(pwee, for pwee support,
[  --with-pwee             Enable environment extensions])

if test "$PHP_PWEE" != "no"; then

  dnl
  dnl If compiled without --with-dom option, we need to include libxml ourself
  dnl

  if test -z "$PHP_DOM" || test "$PHP_DOM" = "no"; then
    PWEE_XML_DIR_ADD=""

    for i in /usr/local /usr; do
      test -r $i/include/libxml2/libxml/tree.h && PWEE_XML_DIR=$i && PWEE_XML_DIR_ADD="/libxml2"
    done

    if test -z "$PWEE_XML_DIR"; then
      AC_MSG_RESULT(not found)
      AC_MSG_ERROR(libxml2 includes not found, please reinstall libxml version 2.4.14 or greater)
    fi

    PWEE_XML_CHECK_VERSION

    XML2_CONFIG=$PWEE_XML_DIR/bin/xml2-config

    if test -x $XML2_CONFIG; then
      DOM_LIBS=`$XML2_CONFIG --libs`
      PHP_EVAL_LIBLINE($DOM_LIBS, PWEE_XML_SHARED_LIBADD)
    else
      AC_MSG_RESULT(not found)
      AC_MSG_ERROR(xml2-config not found, please reinstall libxml version 2.4.14 or greater)
    fi

    PHP_ADD_INCLUDE($PWEE_XML_DIR/include$PWEE_XML_DIR_ADD)
  fi

  AC_CHECK_LIB(uuid, uuid_generate,
	[AC_DEFINE(HAVE_LIBUUID,1,[ ])
		PHP_ADD_LIBRARY(uuid, 1, PWEE_SHARED_LIBADD)],
	[AC_MSG_WARN(using pwee internal version of uuid_generate)])

  AC_MSG_CHECKING([for ioctl])
  AC_TRY_LINK_FUNC(ioctl,
	[AC_DEFINE(HAVE_IOCTL,1,[ ])
		AC_MSG_RESULT([yes])],
	[AC_MSG_ERROR(this version requires ioctl)])

  PHP_NEW_EXTENSION(pwee, pwee.c pwee_conf.c pwee_if.c pwee_uuid.c, $ext_shared)
  PHP_SUBST(PWEE_SHARED_LIBADD)

  ifdef([PHP_ADD_EXTENSION_DEP],
  [
    PHP_ADD_EXTENSION_DEP(pwee, libxml)
  ])
fi
