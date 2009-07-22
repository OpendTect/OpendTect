#ifndef soodbasic_h
#define soodbasic_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2009
 RCS:           $Id: soodbasic.h,v 1.2 2009-07-22 16:01:19 cvsbert Exp $
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
