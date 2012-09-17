#ifndef segybatchio_h
#define segybatchio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		18-10-1995
 Contents:	Selectors
 RCS:		$Id: segybatchio.h,v 1.4 2011/04/21 13:09:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"


/*!\brief

Keys that should be used with od_process_segyio.cc

*/

namespace SEGY
{

namespace IO
{
    inline mGlobal const char* sKeyTask()	{ return "Task"; }
    inline mGlobal const char* sKeyIndexPS()	{ return "Index Pre-Stack"; }
    inline mGlobal const char* sKeyIndex3DVol() { return "Index 3D Volume"; }
    inline mGlobal const char* sKeyIs2D()	{ return "Is 2D"; }

}; //namespace IO

}; //namespce SEGY


#endif
