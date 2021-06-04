#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		18-10-1995
 Contents:	Selectors
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
    inline const char* sKeyExport()	{ return "Export"; }
    inline const char* sKeyImport()	{ return "Import"; }
    inline const char* sKeyIndexPS()	{ return "Index Pre-Stack"; }
    inline const char* sKeyIndex3DVol()	{ return "Index 3D Volume"; }
    inline const char* sKeyIs2D()	{ return "Is 2D"; }
    inline const char* sKeyNullTrcPol()	{ return "Null trace policy"; }
    inline const char* sKeyTask()	{ return "Task"; }

} // namespace IO

} // namespace SEGY


