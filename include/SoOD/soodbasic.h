#ifndef soodbasic_h
#define soodbasic_h
/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          January 2009
 RCS:           $Id: soodbasic.h,v 1.1 2009-02-13 10:47:31 cvsnanne Exp $
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

#endif
