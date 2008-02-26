/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: inituiio.cc,v 1.5 2008-02-26 08:55:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "inituiio.h"
#include "uiposprovgroupstd.h"
#include "uiposfiltgroupstd.h"

void uiIo::initStdClasses()
{
    uiRangePosProvGroup::initClass();
    uiPolyPosProvGroup::initClass();
    uiTablePosProvGroup::initClass();

    uiRandPosFiltGroup::initClass();
    uiSubsampPosFiltGroup::initClass();
}
