/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2002
 RCS:           $Id: horizon3dtracker.cc,v 1.1 2005-12-12 17:26:39 cvskris Exp $
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


void HorizonTracker::interpolateSeeds(const TypeSet<EM::PosID>* posids,
				      EM::SectionID sid)
{
    EM::Horizon* hor = getHorizon();
    EM::HorizonGeometry& geometry = (EM::HorizonGeometry&)hor->geometry;
    const bool didchecksupport = geometry.checkSupport( false );
    for ( int seedidx=1; seedidx<posids->size(); seedidx++ )
    {
	const EM::PosID& pid0 = (*posids)[seedidx-1];
	const EM::PosID& pid1 = (*posids)[seedidx];
	if ( pid0.sectionID()!=sid || pid1.sectionID()!=sid )
	    continue;
	
	BinIDValue nextbiv, curbiv;
	nextbiv.binid.setSerialized(pid1.subID());
	curbiv.binid.setSerialized(pid0.subID());;
	if ( !( nextbiv.binid.inl == curbiv.binid.inl
		|| nextbiv.binid.crl == curbiv.binid.crl ) )
	    continue;
	    
	nextbiv.value = hor->getPos(pid1).z;
	curbiv.value = hor->getPos(pid0).z;
	if ( Values::isUdf(nextbiv.value) || Values::isUdf(curbiv.value) )
	    continue;
	   
	bool inlstick = nextbiv.binid.inl == curbiv.binid.inl;
	BinID step( 0, 0 );
	int nrsteps;
	if ( inlstick )
	{
	    step.crl = curbiv.binid.crl<nextbiv.binid.crl
		? geometry.step().col : -geometry.step().col;
	    nrsteps = (nextbiv.binid.crl-curbiv.binid.crl) / step.crl;
	}
	else
	{
	    step.inl = curbiv.binid.inl<nextbiv.binid.inl
		? geometry.step().row : -geometry.step().row;
	    nrsteps = (nextbiv.binid.inl-curbiv.binid.inl) / step.inl;
	}

	const float dz = (nextbiv.value-curbiv.value) / nrsteps;
	
	for ( int idx = 0; curbiv.binid!=nextbiv.binid; ++idx )
	{
	    const float origz = hor->getPos(sid,curbiv.binid.getSerialized()).z;
	    if ( ! Values::isUdf(origz) && idx ) break;
	    if ( Values::isUdf(origz) ) 
		hor->setPos(sid,curbiv.binid.getSerialized(),
			    Coord3(0,0,curbiv.value),true);
		
	    curbiv.binid += step;
	    curbiv.value += dz;
	}

        if ( seedidx<posids->size()-1 )
	{
	    if (Values::isUdf(hor->getPos(sid,nextbiv.binid.getSerialized()).z))
		hor->setPos(sid,nextbiv.binid.getSerialized(),
				  Coord3(0,0,nextbiv.value),true);
	}
    }

    geometry.checkSupport( didchecksupport );
}




}; // namespace MPE
