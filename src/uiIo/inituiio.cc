/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: inituiio.cc,v 1.2 2008-02-07 16:51:37 cvsbert Exp $
________________________________________________________________________

-*/

#include "inituiio.h"
#include "uiposprovgroupstd.h"

void uiIo::initStdClasses()
{
    uiRangePosProvGroup::initClass();
    uiPolyPosProvGroup::initClass();
}
