# -*- Autoconf -*-

AC_PREREQ(2.53)

define([git_describe], esyscmd([sh -c "git describe --tag | tr -d '\n'"]))dnl
AC_INIT([refine], [git_describe], [fun3d-support@lists.nasa.gov])

dnl AC_INIT([refine], [1.8.21], [fun3d-support@lists.nasa.gov])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_SRCDIR([src/ref_defs.h])
AM_INIT_AUTOMAKE([tar-pax])

AC_ARG_ENABLE(exec,
	[  --enable-exec[=ARG]       build stand alone executables [ARG=yes]],
	[enable_exec=$enableval],    [enable_exec="yes"])

AC_ARG_WITH(SDK,
	[  --with-SDK[=ARG]          use GEOLAB SDK package [ARG=no]],
	[with_SDK=$withval],         [with_SDK="no"])

AC_ARG_WITH(CAPRI,
	[  --with-CAPRI[=ARG]        use CAPRI package [ARG=no]],
	[with_CAPRI=$withval],       [with_CAPRI="no"])

AC_PROG_RANLIB

AC_PROG_CC
AC_HEADER_STDC
AM_PROG_CC_C_O

arch=`uname`

dnl special arch libs

if test "$arch" = 'Linux'
then  
  ARCHlibrary=
  dynamicFlag="-rdynamic"
else
  ARCHlibrary="-lgen"
  dynamicFlag=
fi
AC_SUBST([ARCHlibrary])
AC_SUBST([dynamicFlag])

dnl Enable stand alone executable build

if test "$enable_exec" != 'no'
then
  AM_CONDITIONAL(BUILD_EXECUTABLES,true)
else
  AM_CONDITIONAL(BUILD_EXECUTABLES,false)
fi

dnl turn off mpi stuff in stand alone refine (set by fun3d)
AM_CONDITIONAL(BUILD_MPI,false)
AC_SUBST([MPIINC])
AC_SUBST([mpi_ldadd])

dnl Check for SDK

have_SDK='no'
if test "$with_SDK" != 'no' 
then
  AC_CHECK_FILE([$with_SDK/include/MeatLib/Common.h],
                 [have_SDK='yes'],[have_SDK='no'])
fi

if test "$have_SDK" != 'no'
then
  AC_DEFINE([HAVE_SDK],[1],[Geometry Laboratory SDK available])
  AM_CONDITIONAL(BUILD_SDK_DEPENDENTS,true)
  SDKheader=-I$with_SDK/include
  SDKlibrary=-L$with_SDK/lib
else
  AM_CONDITIONAL(BUILD_SDK_DEPENDENTS,false)
  SDKheader=
  SDKlibrary=
fi
AC_SUBST([SDKheader])
AC_SUBST([SDKlibrary])

dnl Check for X header

AC_PATH_X
if test "$x_includes" != ''
then
  Xheader=-I$x_includes
  Xlibrary=-L$x_libraries
else
  Xheader=
  Xlibrary=
fi
AC_SUBST([Xheader])
AC_SUBST([Xlibrary])

dnl Check for CAPRI header

have_CAPRI='no'
if test "$with_CAPRI" != 'no'
then
  CAPRIinclude=$with_CAPRI/include
  AC_CHECK_FILE([$CAPRIinclude/capri.h],
                [have_CAPRI='yes'],[have_CAPRI='no'])

  if test "$have_CAPRI" != 'no'
  then
    AC_DEFINE([HAVE_CAPRI],[3],[CAPrI 3.0 API])
    CAPRIheader=-I$CAPRIinclude
    CAPRIlibrary="-L$with_CAPRI/lib -lcapriDyn -ldcapri"
    AC_SUBST([CAPRIlibrary])
    AC_SUBST([CAPRIheader])
  else
    AC_MSG_ERROR([capri.h not found in $with_CAPRI/include])
  fi
fi

AC_ARG_WITH(zoltan,
	[  --with-zoltan[=ARG]       use Zoltan partitioner [ARG=no]],
	[with_zoltan=$withval],        [with_zoltan="no"])

if test "$with_zoltan" != 'no'
then
  AC_CHECK_FILE([$with_zoltan/include/zoltan.h],
                [zoltan_h_path=$with_zoltan/include],[zoltan_h_path='no'])
  AC_CHECK_FILE([$with_zoltan/lib/libzoltan.a],
                [zoltan_a_path=$with_zoltan/lib],[zoltan_a_path='no'])
  AC_CHECK_FILE([$with_zoltan/lib/libzoltan.so],
                [zoltan_so_path=$with_zoltan/lib],[zoltan_so_path='no'])
  AC_CHECK_FILE([$with_zoltan/lib/libzoltan.dylib],
                [zoltan_dylib_path=$with_zoltan/lib],[zoltan_dylib_path='no'])

  if test "$zoltan_h_path" != 'no'
  then
    AC_DEFINE([HAVE_ZOLTAN],[1],[Zoltan is available])
    zoltan_include="-I$zoltan_h_path"
  else
    AC_MSG_ERROR([zoltan.h not found in $with_zoltan/include])
  fi
  zoltan_ldadd=""
  if test "$zoltan_a_path" != 'no'
  then
      zoltan_ldadd="-L${zoltan_a_path} -lzoltan"
  fi
  if test "$zoltan_so_path" != 'no'
  then
      zoltan_ldadd="-Wl,-rpath,${zoltan_so_path} -L${zoltan_so_path} -lzoltan"
  fi
  if test "$zoltan_dylib_path" != 'no'
  then
      zoltan_ldadd="-Wl,-rpath,${zoltan_dylib_path} -L${zoltan_dylib_path} -lzoltan"
  fi
  if test "empty$zoltan_ldadd" == 'empty'
  then
    AC_MSG_ERROR([libzoltan.{a,so,dylib} not found in $with_zoltan/lib])
  fi

  AC_SUBST([zoltan_include])
  AC_SUBST([zoltan_ldadd])
fi

AC_ARG_WITH(parmetis,
	[  --with-parmetis[=ARG]     use ParMETIS partitioner [ARG=no]],
	[with_parmetis=$withval],        [with_parmetis="no"])

AC_ARG_WITH(metis,
	[  --with-metis[=ARG]        use METIS partitioner [ARG=ParMETIS path]],
	[with_metis=$withval],        [with_metis="$with_parmetis"])

if test "$with_parmetis" != 'no'
then
  AC_CHECK_FILE([$with_parmetis/include/parmetis.h],
                [parmetis_h_path=$with_parmetis/include],
                [parmetis_h_path='no'])

  AC_CHECK_FILE([$with_parmetis/lib/libparmetis.a],
                [parmetis_a_path=$with_parmetis/lib],
		[parmetis_a_path='no'])
  AC_CHECK_FILE([$with_parmetis/lib/libparmetis.so],
                [parmetis_so_path=$with_parmetis/lib],
		[parmetis_so_path='no'])
  AC_CHECK_FILE([$with_parmetis/lib/libparmetis.dylib],
                [parmetis_dylib_path=$with_parmetis/lib],
		[parmetis_dylib_path='no'])

  AC_CHECK_FILE([$with_metis/include/metis.h],
                [metis_h_path=$with_metis/include],
                [metis_h_path='no'])

  AC_CHECK_FILE([$with_metis/lib/libmetis.a],
                [metis_a_path=$with_metis/lib],
		[metis_a_path='no'])
  AC_CHECK_FILE([$with_metis/lib/libmetis.so],
                [metis_so_path=$with_metis/lib],
		[metis_so_path='no'])
  AC_CHECK_FILE([$with_metis/lib/libmetis.dylib],
                [metis_dylib_path=$with_metis/lib],
		[metis_dylib_path='no'])

  if test "$metis_h_path" != 'no'
  then
    metis_include="-I$metis_h_path"
  else
    AC_MSG_ERROR([metis.h not found in $with_metis/include])
  fi

  metis_ldadd=""
    if test "$metis_a_path" != 'no'
  then
      metis_ldadd="-L${metis_a_path} -lmetis"
  fi
  if test "$metis_so_path" != 'no'
  then
      metis_ldadd="-Wl,-rpath,${metis_so_path} -L${metis_so_path} -lmetis"
  fi
  if test "$metis_dylib_path" != 'no'
  then
      metis_ldadd="-Wl,-rpath,${metis_dylib_path} -L${metis_dylib_path} -lmetis"
  fi
  if test "empty$metis_ldadd" == 'empty'
  then
    AC_MSG_ERROR([libmetis.{a,so,dylib} not found in $with_metis/lib])
  fi

  if test "$parmetis_h_path" != 'no'
  then
    parmetis_include="-I$parmetis_h_path ${metis_include}"
  else
    AC_MSG_ERROR([parmetis.h not found in $with_parmetis/include])
  fi

  parmetis_ldadd=""
    if test "$parmetis_a_path" != 'no'
  then
      parmetis_ldadd="-L${parmetis_a_path} -lparmetis ${metis_ldadd}"
  fi
  if test "$parmetis_so_path" != 'no'
  then
      parmetis_ldadd="-Wl,-rpath,${parmetis_so_path} -L${parmetis_so_path} -lparmetis ${metis_ldadd}"
  fi
  if test "$parmetis_dylib_path" != 'no'
  then
      parmetis_ldadd="-Wl,-rpath,${parmetis_dylib_path} -L${parmetis_dylib_path} -lparmetis ${metis_ldadd}"
  fi
  if test "empty$parmetis_ldadd" == 'empty'
  then
    AC_MSG_ERROR([libparmetis.{a,so,dylib} not found in $with_parmetis/lib])
  fi

  AC_SUBST([parmetis_include])
  AC_SUBST([parmetis_ldadd])
  AC_DEFINE([HAVE_PARMETIS],[1],[ParMETIS is available])
  AM_CONDITIONAL(BUILD_PARMETIS_SUPPORT,true)
else
  AM_CONDITIONAL(BUILD_PARMETIS_SUPPORT,false)
fi

AC_ARG_ENABLE(lite,
        [[  --enable-lite build with EGADSlite [no]]],
        [egadslite=$enableval],[egadslite="no"])

AC_ARG_WITH(EGADS,
	[  --with-EGADS[=ARG]        use EGADS package [ARG=no]],
	[with_egads=$withval],     [with_egads="no"])

if test "$with_egads" != 'no'
then
  if test "$egadslite" != 'no'
  then
    egads_lib_root="egadslite"
  else
    egads_lib_root="egads"
  fi

  AC_CHECK_FILE([$with_egads/include/egads.h],
                [egads_h_path=$with_egads/include],[egads_h_path='no'])

  AC_CHECK_FILE([$with_egads/lib/lib${egads_lib_root}.a],
                [egads_a_path=$with_egads/lib],[egads_a_path='no'])
  AC_CHECK_FILE([$with_egads/lib/lib${egads_lib_root}.so],
                [egads_so_path=$with_egads/lib],[egads_so_path='no'])
  AC_CHECK_FILE([$with_egads/lib/lib${egads_lib_root}.dylib],
                [egads_dylib_path=$with_egads/lib],[egads_dylib_path='no'])

  if test "$egads_h_path" != 'no'
  then
    AC_DEFINE([HAVE_EGADS],[1],[EGADS is available])
    if test "$egadslite" != 'no'
    then
      AC_DEFINE([HAVE_EGADS_LITE],[1],[EGADS is available in lite form])
    fi
    egads_include="-I${egads_h_path}"
  else
    AC_MSG_ERROR([egads.h not found in ${with_egads}/include])
  fi

  egads_ldadd=""
  if test "$egads_a_path" != 'no'
  then
      egads_ldadd="-L${egads_a_path} -l${egads_lib_root}"
  fi
  if test "$egads_so_path" != 'no'
  then
      egads_ldadd="-Wl,-rpath,${egads_so_path} -L${egads_so_path} -l${egads_lib_root}"
  fi
  if test "$egads_dylib_path" != 'no'
  then
      egads_ldadd="-Wl,-rpath,${egads_dylib_path} -L${egads_dylib_path} -l${egads_lib_root}"
  fi
  if test "empty$egads_ldadd" == 'empty'
  then
    AC_MSG_ERROR([lib${egads_lib_root}.{a,so,dylib} not found in $with_egads/lib])
  fi

  AC_SUBST([egads_include])
  AC_SUBST([egads_ldadd])
fi

AC_ARG_WITH(OpenCASCADE,
	[  --with-OpenCASCADE[=ARG]  use OpenCASCADE CAD Kernel [ARG=no]],
	[with_OpenCASCADE=$withval],        [with_OpenCASCADE="no"])

if test "$with_OpenCASCADE" != 'no'
then
  AC_CHECK_FILE([$with_OpenCASCADE/lib/libTKernel.a],
                [opencascade_a_path=$with_OpenCASCADE/lib],[opencascade_a_path='no'])
  AC_CHECK_FILE([$with_OpenCASCADE/lib/libTKernel.so],
                [opencascade_so_path=$with_OpenCASCADE/lib],[opencascade_so_path='no'])
  AC_CHECK_FILE([$with_OpenCASCADE/lib/libTKernel.dylib],
                [opencascade_dylib_path=$with_OpenCASCADE/lib],[opencascade_dylib_path='no'])

  opencascade_ldadd=""
  if test "$opencascade_a_path" != 'no'
  then
      opencascade_ldadd="-L${opencascade_a_path}"
  fi
  if test "$opencascade_so_path" != 'no'
  then
      opencascade_ldadd="-Wl,-rpath,${opencascade_so_path} -L${opencascade_so_path}"
  fi
  if test "$opencascade_dylib_path" != 'no'
  then
      opencascade_ldadd="-Wl,-rpath,${opencascade_dylib_path} -L${opencascade_dylib_path}"
  fi
  if test "empty$opencascade_ldadd" == 'empty'
  then
    AC_MSG_ERROR([libTKernel.{a,so,dylib} for OpenCASCADE not found in $with_OpenCASCADE/lib])
  fi

  AC_SUBST([opencascade_ldadd])
fi

AC_CONFIG_FILES( \
	   Makefile \
	   src/Makefile \
	   one/Makefile \
	   util/Makefile
	   )
AC_CONFIG_FILES( \
	   acceptance/Makefile
	   )

AC_OUTPUT
