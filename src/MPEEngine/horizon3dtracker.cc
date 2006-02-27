/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2002
 RCS:           $Id: horizon3dtracker.cc,v 1.2 2006-02-27 12:15:24 cvsjaap Exp $
________________________________________________________________________

-*/

#include "horizontracker.h"

#include "cubicbeziercurve.h"
#include "emhistory.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "horizonadjuster.h"
#include "horizonextender.h"
#include "horizonseedpicker.h"
#include "mpeengine.h"
#include "sectionselectorimpl.h"
#include "sectiontracker.h"
#include "consistencychecker.h"
#include "survinfo.h"


#include <math.h>

namespace MPE
{

HorizonTracker::HorizonTracker( EM::Horizon* hor )
    : EMTracker(hor)
    , consistencychecker(0)
    , seedpicker( 0 )
{}


HorizonTracker::~HorizonTracker()
{
    delete seedpicker;
}


EMTracker* HorizonTracker::create( EM::EMObject* emobj )
{
    mDynamicCastGet(EM::Horizon*,hor,emobj)
    return emobj && !hor ? 0 : new HorizonTracker( hor );
}


void HorizonTracker::initClass()
{
    MPE::engine().addTrackerFactory(
	    new TrackerFactory( EM::Horizon::typeStr(), create ) );
}


#define mErrRet(msg) { errmsg = msg; return false; }

SectionTracker* HorizonTracker::createSectionTracker( EM::SectionID sid )
{
    if ( !getHorizon() ) return 0;

    return new SectionTracker( *emObject(), sid,
	    new BinIDSurfaceSourceSelector(*getHorizon(),sid),
	    new HorizonExtender(*getHorizon(),sid),
	    new HorizonAdjuster(*getHorizon(),sid) );
}


bool HorizonTracker::trackIntersections( const TrackPlane& plane )
{
    return true;
}


EMSeedPicker*  HorizonTracker::getSeedPicker(bool createifnotpresent)
{
    if ( seedpicker )
	return seedpicker;

    if ( !createifnotpresent )
	return 0;

    seedpicker = new HorizonSeedPicker(*this);
    return seedpicker;
}


EM::Horizon* HorizonTracker::getHorizon()
{
    mDynamicCastGet(EM::Horizon*,hor,emObject());
    return hor;
}


const EM::Horizon* HorizonTracker::getHorizon() const 
{ return const_cast<HorizonTracker*>(this)->getHorizon(); }


ConsistencyChecker* HorizonTracker::getConsistencyChecker()
{
    mDynamicCastGet(EM::Horizon*,horizon,emObject());
    if ( !horizon ) return 0;

    if ( !consistencychecker )
	consistencychecker = new ConsistencyChecker( *horizon );

    return consistencychecker;
}

}; // namespace MPE
