/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          December 2007
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: inituiprestackprocessing.cc,v 1.9 2012-05-02 11:53:49 cvskris Exp $";


#include "moddepmgr.h"
#include "uiprestackagc.h"
#include "uiprestackmute.h"
#include "uiprestacklateralstack.h"
#include "uiprestackanglemute.h"

mDefModInitFn(uiPreStackProcessing)
{
    mIfNotFirstTime( return );

    PreStack::uiAGC::initClass();
    PreStack::uiMute::initClass();
    PreStack::uiLateralStack::initClass();
    PreStack::uiAngleMute::initClass();
}
