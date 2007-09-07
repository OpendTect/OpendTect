#ifndef coltab_h
#define coltab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: coltab.h,v 1.1 2007-09-07 11:21:01 cvsbert Exp $
________________________________________________________________________

-*/


#include "color.h"

namespace ColTab
{

float	defClipRate();
float	defSymMidval();
void	setMapperDefaults(float cr,float sm);

}


#endif
