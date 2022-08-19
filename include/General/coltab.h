#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "color.h"
#include "ranges.h"

/*!\brief %Color %Table */

namespace ColTab
{

    mGlobal(General) const char*    defSeqName();
    mGlobal(General) Interval<float>defClipRate();
    mGlobal(General) float	    defSymMidval();
    mGlobal(General) bool	    defAutoSymmetry();
    mGlobal(General) void	    setMapperDefaults(Interval<float> cr,
						      float sm,bool autosym,
						      bool histeq=false);
    mGlobal(General) bool	    defHistEq();
}
