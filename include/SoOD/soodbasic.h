#ifndef soodbasic_h
#define soodbasic_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2009
 RCS:           $Id: soodbasic.h,v 1.14 2012-08-27 13:16:49 cvskris Exp $
 ________________________________________________________________________

-*/

#include "soodmod.h"

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

#define mSoODGlobal 		Export_SoOD
#define mSoODClass		class mSoODGlobal
#define mSoODExternC		extern "C" mSoODGlobal

#endif

