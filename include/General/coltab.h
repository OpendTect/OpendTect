#ifndef coltab_h
#define coltab_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: coltab.h,v 1.8 2011/02/10 05:11:27 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "color.h"
#include "ranges.h"

namespace ColTab
{

    mGlobal const char*	    defSeqName();
    mGlobal Interval<float> defClipRate();
    mGlobal float	    defSymMidval();
    mGlobal bool	    defAutoSymmetry();
    mGlobal void	    setMapperDefaults(Interval<float> cr,float sm,
	    				      bool autosym,bool histeq=false);
    mGlobal bool	    defHistEq();
}


#endif
