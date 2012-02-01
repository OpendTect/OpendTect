#ifndef soodbasic_h
#define soodbasic_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2009
 RCS:           $Id: soodbasic.h,v 1.5 2012-02-01 19:56:07 cvsyuancheng Exp $
 ________________________________________________________________________

-*/

#define dll_export
#if defined(WIN32)
# undef __win__
# define __win__ 1
# if defined(SOOD_EXPORTS)
#  define dll_export __declspec( dllexport )
# endif
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
