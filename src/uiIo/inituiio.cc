/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiio.cc,v 1.7 2009-07-22 16:01:39 cvsbert Exp $";

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
