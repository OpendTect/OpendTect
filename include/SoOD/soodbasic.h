#ifndef soodbasic_h
#define soodbasic_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2009
 RCS:           $Id: soodbasic.h,v 1.12 2012-08-03 21:56:26 cvskris Exp $
 ________________________________________________________________________

-*/

//#include "soodmod.h"

//Run own import/export until we enabled it globally
#ifndef dll_export
# if defined( __win64__ ) || defined ( __win32__ )
#  define dll_export	__declspec( dllexport )
#  define dll_import	__declspec( dllimport )
# else
#  define dll_export
#  define dll_import
# endif
#endif
#if defined(SoOD_EXPORTS) || defined(SOOD_EXPORTS)
# define Export_SoOD	dll_export
#else
# define Export_SoOD	dll_import
#endif

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

# define mExportClass( module ) class Export_##module


#ifndef mGlobal
# define mGlobal( module ) 		Export_##module
#endif

#ifndef mClass
# define mClass( module )		class mGlobal( module )
#endif


#ifndef mExternC
#define mExternC( module )		extern "C" mGlobal( module )
#endif


#endif

