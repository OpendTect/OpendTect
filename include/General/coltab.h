#ifndef coltab_h
#define coltab_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: coltab.h,v 1.7 2009-07-22 16:01:15 cvsbert Exp $
________________________________________________________________________

-*/


#include "color.h"

namespace ColTab
{

    mGlobal const char*	    defSeqName();
    mGlobal float	    defClipRate();
    mGlobal float	    defSymMidval();
    mGlobal bool	    defAutoSymmetry();
    mGlobal void	    setMapperDefaults(float cr,float sm,bool autosym,
	    		    bool histeq=false);
    mGlobal bool	    defHistEq();
}


#endif
