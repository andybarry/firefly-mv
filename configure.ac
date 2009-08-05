AC_INIT([firefly-mv-utils], [0.1], [john.stowers@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_DISABLE_SHARED

PKG_CHECK_MODULES(DC1394, libdc1394-2 >= 2.1)
AC_SUBST(DC1394_CFLAGS)
AC_SUBST(DC1394_LIBS)

PKG_CHECK_MODULES(GLIB, glib-2.0)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

AC_ARG_ENABLE([gtk], [  --enable-gtk  build gtk+ programs],[
    PKG_CHECK_MODULES(GTK, gtk+-2.0)
    AC_SUBST(GTK_CFLAGS)
    AC_SUBST(GTK_LIBS)
    enable_gtk=$enableval
    ],
    enable_gtk=no)
AM_CONDITIONAL([ENABLE_GTK], [test x$enable_gtk = xyes])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT