/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: inituiio.cc,v 1.9 2011-08-23 06:54:12 cvsbert Exp $";

#include "inituiio.h"
#include "uiposprovgroupstd.h"
#include "uiposfiltgroupstd.h"
#include "uit2dconvsel.h"

void uiIo::initStdClasses()
{
    mIfNotFirstTime( return );

    uiRangePosProvGroup::initClass();
    uiPolyPosProvGroup::initClass();
    uiTablePosProvGroup::initClass();

    uiRandPosFiltGroup::initClass();
    uiSubsampPosFiltGroup::initClass();

    uiT2DLinConvSelGroup::initClass();
}
