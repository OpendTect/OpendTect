#ifndef batchmod_h
#define batchmod_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id$
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

#if defined(Batch_EXPORTS) || defined(BATCH_EXPORTS)
# define do_export_Batch
#else
# if defined ( do_export_Batch )
#  undef do_export_Batch
# endif
#endif


#if defined( do_export_Batch )
# define Export_Batch	dll_export
# define Extern_Batch
#else
# define Export_Batch	dll_import
# define Extern_Batch	dll_extern
#endif

#if defined ( do_import_export )

//#include "networkmod.h"

#endif

#endif

