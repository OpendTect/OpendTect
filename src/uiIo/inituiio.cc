/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: inituiio.cc,v 1.3 2008-02-08 09:56:17 cvsbert Exp $
________________________________________________________________________

-*/

#include "inituiio.h"
#include "uiposprovgroupstd.h"

void uiIo::initStdClasses()
{
    uiRangePosProvGroup::initClass();
    uiPolyPosProvGroup::initClass();
    uiTablePosProvGroup::initClass();
}
