# ===========================================================================
#      http://www.gnu.org/software/autoconf-archive/ax_boost_iostreams.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_BOOST_IOSTREAMS
#
# DESCRIPTION
#
#   Test for System library from the Boost C++ libraries. The macro requires
#   a preceding call to AX_BOOST_BASE. Further documentation is available at
#   <http://randspringer.de/boost/index.html>.
#
#   This macro calls:
#
#     AC_SUBST(BOOST_IOSTREAMS_LIB)
#
#   And sets:
#
#     HAVE_BOOST_IOSTREAMS
#
# LICENSE
#
#   Copyright (c) 2012 Xiyue Deng <manphiz@gmail.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 1

AC_DEFUN([AX_BOOST_IOSTREAMS],
[
	AC_ARG_WITH([boost-iostreams],
	AS_HELP_STRING([--with-boost-iostreams@<:@=special-lib@:>@],
                   [use the IoStreams library from boost - it is possible to specify a certain library for the linker
                        e.g. --with-boost-iostreams=boost_iostreams-gcc-mt ]),
        [
        if test "$withval" = "no"; then
			want_boost="no"
        elif test "$withval" = "yes"; then
            want_boost="yes"
            ax_boost_user_iostreams_lib=""
        else
		    want_boost="yes"
		ax_boost_user_iostreams_lib="$withval"
		fi
        ],
        [want_boost="yes"]
	)

	if test "x$want_boost" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
        AC_REQUIRE([AC_CANONICAL_BUILD])
		CPPFLAGS_SAVED="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
		export CPPFLAGS

		LDFLAGS_SAVED="$LDFLAGS"
		LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
		export LDFLAGS

        AC_CACHE_CHECK(whether the Boost::IoStreams library is available,
					   ax_cv_boost_iostreams,
        [AC_LANG_PUSH([C++])
			 CXXFLAGS_SAVE=$CXXFLAGS

			 AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[@%:@include <boost/iostreams.hpp>]],
                                   [[boost::iostreams::system_clock::time_point time;]])],
                   ax_cv_boost_iostreams=yes, ax_cv_boost_iostreams=no)
			 CXXFLAGS=$CXXFLAGS_SAVE
             AC_LANG_POP([C++])
		])
		if test "x$ax_cv_boost_iostreams" = "xyes"; then
			AC_SUBST(BOOST_CPPFLAGS)

			AC_DEFINE(HAVE_BOOST_IOSTREAMS,,[define if the Boost::IoStreams library is available])
            BOOSTLIBDIR=`echo $BOOST_LDFLAGS | sed -e 's/@<:@^\/@:>@*//'`

			LDFLAGS_SAVE=$LDFLAGS
            if test "x$ax_boost_user_iostreams_lib" = "x"; then
                ax_lib=
                for libextension in `ls $BOOSTLIBDIR/libboost_iostreams*.so* $BOOSTLIBDIR/libboost_iostreams*.dylib* $BOOSTLIBDIR/libboost_iostreams*.a* 2>/dev/null | sed 's,.*/,,' | sed -e 's;^lib\(boost_iostreams.*\)\.so.*$;\1;' -e 's;^lib\(boost_iostreams.*\)\.dylib.*$;\1;' -e 's;^lib\(boost_iostreams.*\)\.a.*$;\1;'` ; do
                     ax_lib=${libextension}
				    AC_CHECK_LIB($ax_lib, exit,
                                 [BOOST_IOSTREAMS_LIB="-l$ax_lib"; AC_SUBST(BOOST_IOSTREAMS_LIB) link_iostreams="yes"; break],
                                 [link_iostreams="no"])
				done
                if test "x$link_iostreams" != "xyes"; then
                for libextension in `ls $BOOSTLIBDIR/boost_iostreams*.dll* $BOOSTLIBDIR/boost_iostreams*.a* 2>/dev/null | sed 's,.*/,,' | sed -e 's;^\(boost_iostreams.*\)\.dll.*$;\1;' -e 's;^\(boost_iostreams.*\)\.a.*$;\1;'` ; do
                     ax_lib=${libextension}
				    AC_CHECK_LIB($ax_lib, exit,
                                 [BOOST_IOSTREAMS_LIB="-l$ax_lib"; AC_SUBST(BOOST_IOSTREAMS_LIB) link_iostreams="yes"; break],
                                 [link_iostreams="no"])
				done
                fi

            else
               for ax_lib in $ax_boost_user_iostreams_lib boost_iostreams-$ax_boost_user_iostreams_lib; do
				      AC_CHECK_LIB($ax_lib, exit,
                                   [BOOST_IOSTREAMS_LIB="-l$ax_lib"; AC_SUBST(BOOST_IOSTREAMS_LIB) link_iostreams="yes"; break],
                                   [link_iostreams="no"])
                  done

            fi
            if test "x$ax_lib" = "x"; then
                AC_MSG_ERROR(Could not find a version of the boost_iostreams library!)
            fi
			if test "x$link_iostreams" = "xno"; then
				AC_MSG_ERROR(Could not link against $ax_lib !)
			fi
		fi

		CPPFLAGS="$CPPFLAGS_SAVED"
	LDFLAGS="$LDFLAGS_SAVED"
	fi
])
