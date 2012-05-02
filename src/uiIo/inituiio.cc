/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: inituiio.cc,v 1.11 2012-05-02 11:53:43 cvskris Exp $";

#include "moddepmgr.h"
#include "uiposprovgroupstd.h"
#include "uiposfiltgroupstd.h"
#include "uit2dconvsel.h"

mDefModInitFn(uiIo)
{
    mIfNotFirstTime( return );

    uiRangePosProvGroup::initClass();
    uiPolyPosProvGroup::initClass();
    uiTablePosProvGroup::initClass();

    uiRandPosFiltGroup::initClass();
    uiSubsampPosFiltGroup::initClass();

    uiT2DLinConvSelGroup::initClass();
}
