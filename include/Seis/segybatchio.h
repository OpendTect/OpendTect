#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
