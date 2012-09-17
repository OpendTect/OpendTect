#ifndef soodbasic_h
#define soodbasic_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2009
 RCS:           $Id: soodbasic.h,v 1.8 2012/07/20 06:52:08 cvskris Exp $
 ________________________________________________________________________

-*/

#if defined( WIN32 ) || defined( win32 ) || defined( win64 )
# undef __win__
# define __win__ 1
# if defined(SOOD_EXPORTS) || defined(SoOD_EXPORTS)
#  define Export_SoOD __declspec( dllexport )
# else
#  define Export_SoOD __declspec( dllimport )
# endif
#else
# define Export_SoOD
#endif

#if defined( WIN32 ) || defined( win32 ) || defined( win64 )
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
