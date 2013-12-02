#ifndef segybatchio_h
#define segybatchio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		18-10-1995
 Contents:	Selectors
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "gendefs.h"


/*!\brief Keys that should be used with od_process_segyio.cc */

namespace SEGY
{

/*!\brief Input/Output*/

namespace IO
{
    inline const char* sProgname()	{ return "od_process_segyio"; }
    inline const char* sKeyTask()	{ return "Task"; }
    inline const char* sKeyImport()	{ return "Import"; }
    inline const char* sKeyIndexPS()	{ return "Index Pre-Stack"; }
    inline const char* sKeyIndex3DVol()	{ return "Index 3D Volume"; }
    inline const char* sKeyIs2D()	{ return "Is 2D"; }
    inline const char* sKeyNullTrcPol()	{ return "Null trace policy"; }

}; //namespace IO

}; //namespce SEGY


#endif

