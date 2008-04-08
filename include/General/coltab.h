#ifndef coltab_h
#define coltab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: coltab.h,v 1.2 2008-04-08 06:33:48 cvsnanne Exp $
________________________________________________________________________

-*/


#include "color.h"

namespace ColTab
{

    const char*		defSeqName();
    float		defClipRate();
    float		defSymMidval();
    void		setMapperDefaults(float cr,float sm);

}


#endif
