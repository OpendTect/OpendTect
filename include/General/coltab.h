#ifndef coltab_h
#define coltab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: coltab.h,v 1.5 2008-11-24 10:45:29 cvsnanne Exp $
________________________________________________________________________

-*/


#include "color.h"

namespace ColTab
{

    const char*		defSeqName();
    float		defClipRate();
    float		defSymMidval();
    bool		defAutoSymmetry();
    void		setMapperDefaults(float cr,float sm,bool autosym,
	    				  bool histeq=false);
    bool		defHistEq();
}


#endif
