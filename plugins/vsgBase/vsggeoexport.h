#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

 vsgGeo is built into the vsgBase plugin, but must not depend on
 OpendTect: this replaces vsgbasemod.h, which pulls in all OD module headers.

-*/

#if defined( _WIN32 )
# if defined( vsgBase_EXPORTS ) || defined( VSGBASE_EXPORTS )
#  define VSGGEO_EXPORT __declspec( dllexport )
# else
#  define VSGGEO_EXPORT __declspec( dllimport )
# endif
#else
# define VSGGEO_EXPORT
#endif
