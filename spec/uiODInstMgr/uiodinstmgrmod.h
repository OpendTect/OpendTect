#ifndef uiodinstmgrmod_h
#define uiodinstmgrmod_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id: initheader.h.in 28999 2013-03-26 13:38:47Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________


-*/

#if defined( __win64__ ) || defined ( __win32__ )
# define do_import_export
#else
# ifdef do_import_export
#  undef do_import_export
# endif
#endif

#ifndef dll_export
# if defined( do_import_export )
#  define dll_export	__declspec( dllexport )
#  define dll_import	__declspec( dllimport )
#  define dll_extern	extern
# else
#  define dll_export
#  define dll_import
# endif
#endif

#if defined(uiODInstMgr_EXPORTS) || defined(UIODINSTMGR_EXPORTS)
# define do_export_uiODInstMgr
#else
# if defined ( do_export_uiODInstMgr )
#  undef do_export_uiODInstMgr
# endif
#endif


#if defined( do_export_uiODInstMgr )
# define Export_uiODInstMgr	dll_export
# define Extern_uiODInstMgr
#else
# define Export_uiODInstMgr	dll_import
# define Extern_uiODInstMgr	dll_extern
#endif

#if defined ( do_import_export )
//Temporary allow extern declaration of extern template instantiation
#pragma warning( push )
#pragma warning( disable : 4231 )

#pragma warning( pop )

#include "uicmddrivermod.h"

#endif

#endif
