#ifndef soodbasic_h
#define soodbasic_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2009
 RCS:           $Id: soodbasic.h,v 1.3 2011-08-29 05:50:50 cvsranojay Exp $
 ________________________________________________________________________

-*/

#if defined(WIN32) && defined(SOOD_EXPORTS)
# define dll_export __declspec( dllexport )
#else
# define dll_export
#endif

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
