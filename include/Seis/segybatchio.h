#ifndef segybatchio_h
#define segybatchio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		18-10-1995
 Contents:	Selectors
 RCS:		$Id: segybatchio.h,v 1.5 2012-08-03 13:00:34 cvskris Exp $
________________________________________________________________________

-*/

#include "seismod.h"
#include "gendefs.h"


/*!\brief

Keys that should be used with od_process_segyio.cc

*/

namespace SEGY
{

namespace IO
{
    inline mGlobal(Seis) const char* sKeyTask()	{ return "Task"; }
    inline mGlobal(Seis) const char* sKeyIndexPS()	{ return "Index Pre-Stack"; }
    inline mGlobal(Seis) const char* sKeyIndex3DVol() { return "Index 3D Volume"; }
    inline mGlobal(Seis) const char* sKeyIs2D()	{ return "Is 2D"; }

}; //namespace IO

}; //namespce SEGY


#endif

