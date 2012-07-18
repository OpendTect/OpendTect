#ifndef soodbasic_h
#define soodbasic_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2009
 RCS:           $Id: soodbasic.h,v 1.8 2012-07-18 09:56:51 cvskris Exp $
 ________________________________________________________________________

-*/

#if defined( __win64__ ) || defined ( __win32__ )
# define __win__ 1
#endif

#if defined ( __lux64__ ) || defined ( __lux32__ )
# define __unix__ 1
# define __lux__ 1
#endif

#if defined( __mac__ )
# define __unix__ 1
#endif

#ifndef __unix__
#ifndef __win__
# error "Platform not detected."
#endif
#endif


#if defined(__win__)
# if defined(SOOD_EXPORTS) || defined(SoOD_EXPORTS)
#  define Export_SoOD __declspec( dllexport )
# else
#  define Export_SoOD __declspec( dllimport )
# endif
#else
# define Export_SoOD
#endif

#if defined(__win__)
# if defined(SOOD_EXPORTS) || defined(SoOD_EXPORTS)
#  define dll_export __declspec( dllexport )
# endif
#else
# ifndef dll_export
#  define dll_export
# endif
#endif



# define mExportClass( module ) class Export_##module


#ifndef mClass
# define mClass class dll_export
#endif

#ifndef mGlobal
# define mGlobal dll_export
#endif

#ifndef mExternC
#define mExternC	extern "C" dll_export
#endif


#endif
