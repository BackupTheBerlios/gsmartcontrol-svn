
############################################################################
# Copyright:
#      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
# License: See LICENSE_zlib.txt file
############################################################################


# APP_CHOOSE_GLD_READER([C|C++][, flags_prefix])
# This macro detects whether GtkBuilder (from gtk >= 2.12) or libglade
# are present. If the first parameter is C++, then gtkmm and libglademm
# are searched for instead.
# flags_prefixCFLAGS, flags_prefixCXXFLAGS and flags_prefixLIBS
# will be initialized (or appended to) with the correct flags in case of libglade(mm).
# app_cv_gld_reader will be set to none, libglade or gtkbuilder.

# ENABLE_LIBGLADE or ENABLE_GTKBUILDER will be exported to config.h

# You must call AX_COMPILER_VENDOR before using this macro.

AC_DEFUN([APP_CHOOSE_GLD_READER], [
	# These requirements check compile-time presence, not that they were called
	AC_REQUIRE([AX_COMPILER_VENDOR])

	# This is just in case
	# PKG_PROG_PKG_CONFIG()

	# this variable will have values "none", "libglade" or "gtkbuilder".
	app_cv_gld_reader="none";

	# in case libglade(mm) is found, these will contain the flags.
	app_cv_gld_reader_CFLAGS="";
	app_cv_gld_reader_CXXFLAGS="";
	app_cv_gld_reader_LIBS="";

	app_cv_gld_reader_libglade_name="libglade";
	app_cv_gld_reader_libglade_pkg_name="libglade-2.0";
	app_cv_gld_reader_gtk_name="gtk";
	app_cv_gld_reader_gtk_pkg_name="gtk+-2.0";
	if test "x$1" = "xC++"; then
		app_cv_gld_reader_libglade_name="libglademm";
		app_cv_gld_reader_libglade_pkg_name="libglademm-2.4";
		app_cv_gld_reader_gtk_name="gtkmm";
		app_cv_gld_reader_gtk_pkg_name="gtkmm-2.4";
	fi


	# Manual control. AC_ARG_ENABLE doesn't get bash variables, so we have
	# to do it this way. If put inside a shell conditional, it still shows up in configure help.

	AC_ARG_ENABLE(libglade, AS_HELP_STRING([--enable-libglade],
			[use libglade(mm) instead of GtkBuilder (default: auto)]),
			[with_libglade=${enableval}], [with_libglade=auto])


	# Try gtkbuilder first
	if test "x$with_libglade" = "xno" || test "x$with_libglade" = "xauto"; then
		PKG_CHECK_EXISTS([$app_cv_gld_reader_gtk_pkg_name >= 2.12.0],
				[app_cv_gld_reader="gtkbuilder"])
	fi
	if test "$with_libglade" = "no" && test "$app_cv_gld_reader" = "none"; then
		AC_MSG_ERROR([GtkBuilder support not found (you need $app_cv_gld_reader_gtk_name version 2.12 or higher). Try using $app_cv_gld_reader_libglade_name instead.])
	fi

	# libglade specified, or auto and no gtkbuilder. gtkmm2.4-compatible libglademm is >= 2.4.0.
	# (not sure about libglade version).
	if test "x$app_cv_gld_reader" = "xnone"; then
		PKG_CHECK_MODULES([app_cv_gld_reader], [$app_cv_gld_reader_libglade_pkg_name >= 2.4.0],
				[app_cv_gld_reader="libglade"], [AC_MSG_WARN([$LIBGLADE_PKG_ERRORS])])
	fi

	if test "x$with_libglade" = "xyes" && test "x$app_cv_gld_reader" = "xnone"; then
		AC_MSG_ERROR([$app_cv_gld_reader_libglade_name not found. Try building with GtkBuilder support (you need $app_cv_gld_reader_gtk_name version 2.12 or higher).])
	fi

	# still not found.
	if test "$app_cv_gld_reader" = "none"; then
		AC_MSG_WARN([Neither GtkBuilder nor $app_cv_gld_reader_libglade_name found. The program will be unable to compile fully.])
	fi


	# pkg-config doesn't do CXXFLAGS, so do it manually.
	app_cv_gld_reader_CXXFLAGS="$app_cv_gld_reader_CFLAGS";

	# if using libglademm, it doesn't make any sense to leave the flags for C.
	if test "x$1" = "xC++"; then
		app_cv_gld_reader_CFLAGS="";
	fi


	# export the flags
	if test "x$2" != "x"; then
		if test "x$$2[]CFLAGS" = "x"; then
			$2[]CFLAGS="$app_cv_gld_reader_CFLAGS";
		else
			$2[]CFLAGS="$$2[]CFLAGS $app_cv_gld_reader_CFLAGS";
		fi
		if test "x$$2[]CXXFLAGS" = "x"; then
			$2[]CXXFLAGS="$app_cv_gld_reader_CXXFLAGS";
		else
			$2[]CXXFLAGS="$$2[]CXXFLAGS $app_cv_gld_reader_CXXFLAGS";
		fi
		if test "x$$2[]LIBS" = "x"; then
			$2[]LIBS="$app_cv_gld_reader_LIBS";
		else
			$2[]LIBS="$$2[]LIBS $app_cv_gld_reader_LIBS";
		fi
	fi


	# export to automake.am-s
	AM_CONDITIONAL([ENABLE_LIBGLADE], [test "$app_cv_gld_reader" = "libglade"])
	AM_CONDITIONAL([ENABLE_GTKBUILDER], [test "$app_cv_gld_reader" = "gtkbuilder"])


	# export to config.h
	if test "$app_cv_gld_reader" = "libglade"; then
		AC_DEFINE(ENABLE_LIBGLADE, 1, [Use libglade(mm) instead of GtkBuilder])
	fi
	if test "$app_cv_gld_reader" = "gtkbuilder"; then
		AC_DEFINE(ENABLE_GTKBUILDER, 1, [Use GtkBuilder instead of libglademm])
	fi

	# If we move this to the start, some messages get in between, so we have to have it here.
	AC_MSG_CHECKING([for glade file support:])

	AC_MSG_RESULT([$app_cv_gld_reader])

])





