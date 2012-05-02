/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: inituiseis.cc,v 1.6 2012-05-02 15:12:15 cvskris Exp $";

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
