#ifndef batchmod_h
#define batchmod_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id: batchmod.h,v 1.2 2012-08-30 05:05:57 cvskris Exp $
________________________________________________________________________


-*/

#ifndef dll_export
# if defined ( __win64__ ) || defined ( __win32__ )
#  define dll_export	__declspec( dllexport )
#  define dll_import	__declspec( dllimport )
# else
#  define dll_export
#  define dll_import
# endif
#endif

#if defined(Batch_EXPORTS) || defined(BATCH_EXPORTS)
# define Export_Batch	dll_export
#else
# define Export_Batch	dll_import
#endif


#endif
