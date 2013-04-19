#ifndef coltab_h
#define coltab_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id$
________________________________________________________________________

-*/


#include "generalmod.h"
#include "color.h"
#include "ranges.h"

/*!\brief Color Table */

namespace ColTab
{

    mGlobal(General) const char*    defSeqName();
    mGlobal(General) Interval<float>defClipRate();
    mGlobal(General) float	    defSymMidval();
    mGlobal(General) bool	    defAutoSymmetry();
    mGlobal(General) void	    setMapperDefaults(Interval<float>cr,float sm					,bool autosym,bool histeq=false);
    mGlobal(General) bool	    defHistEq();
}


#endif

