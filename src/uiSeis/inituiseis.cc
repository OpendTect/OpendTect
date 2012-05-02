/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: inituiseis.cc,v 1.5 2012-05-02 11:53:50 cvskris Exp $";

#include "moddepmgr.h"
#include "uiveldesc.h"
#include "uit2dvelconvselgroup.h"

mDefModInitFn(uiSeis)
{
    mIfNotFirstTime( return );

    uiTime2Depth::initClass();
    uiDepth2Time::initClass();
    uiT2DVelConvSelGroup::initClass();
}
