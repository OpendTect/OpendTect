#ifndef coltab_h
#define coltab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: coltab.h,v 1.6 2009-01-20 04:38:46 cvsranojay Exp $
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
