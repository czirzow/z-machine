dnl $Id: config.m4,v 1.1 2000/07/09 15:05:21 curt Exp $
dnl config.m4 for extension frotz
dnl don't forget to call PHP_EXTENSION(frotz)


PHP_ARG_WITH(frotz,for frotz support,
[  --with-frotz   Include frotz 'z-machine' support)

if test "$PHP_FROTZ" != "no"; then

  AC_DEFINE(HAVE_LIBFROTZ, 1, [ ])
  PHP_EXTENSION(frotz, $ext_shared)
  LIB_BUILD($ext_builddir/Z,$ext_shared,yes)
  AC_ADD_INCLUDE($ext_srcdir/Z)
  PHP_FAST_OUTPUT($ext_builddir/Z/Makefile)

fi
