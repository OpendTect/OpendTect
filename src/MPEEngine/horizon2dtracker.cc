/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizon2dtracker.h"

#include "emhorizon2d.h"
#include "emmanager.h"
#include "horizonadjuster.h"
#include "horizon2dextender.h"
#include "horizon2dseedpicker.h"
#include "horizon2dselector.h"
#include "sectiontracker.h"

namespace MPE
{

RefMan<Horizon2DTracker> Horizon2DTracker::create( EM::Horizon2D& hor )
{
    return new Horizon2DTracker( hor );
}


Horizon2DTracker::Horizon2DTracker( EM::Horizon2D& hor )
    : EMTracker(hor)
{
    setTypeStr( EM::Horizon2D::typeStr() );
}


Horizon2DTracker::~Horizon2DTracker()
{
    delete seedpicker_;
}

// following override function implementations are added if ever we need them

bool Horizon2DTracker::hasTrackingMgr() const
{
    return EMTracker::hasTrackingMgr();
}


bool Horizon2DTracker::createMgr()
{
    return EMTracker::createMgr();
}


void Horizon2DTracker::startFromSeeds( const TypeSet<TrcKey>& seeds )
{
    EMTracker::startFromSeeds( seeds );
}


void Horizon2DTracker::initTrackingMgr()
{
    EMTracker::initTrackingMgr();
}


bool Horizon2DTracker::trackingInProgress() const
{
    return EMTracker::trackingInProgress();
}


void Horizon2DTracker::updateFlatCubesContainer(
				const TrcKeyZSampling& tkzs, bool addremove )
{
    EMTracker::updateFlatCubesContainer( tkzs, addremove );
}


void Horizon2DTracker::stopTracking()
{
    EMTracker::stopTracking();
}


#define mErrRet(msg) { errmsg = msg; return false; }

SectionTracker* Horizon2DTracker::createSectionTracker()
{
    RefMan<EM::EMObject> emobject = emObject();
    mDynamicCastGet(EM::Horizon2D*,hor2d,emobject.ptr());
    if ( !hor2d )
	return nullptr;

    Horizon2DExtenderBase* extender2d =
			Horizon2DExtenderBase::createInstance( *hor2d );

    EM::EMObject* emobj = emobject.ptr();
    return new SectionTracker( *emobj, new Horizon2DSelector(*hor2d),
			       extender2d, new HorizonAdjuster(*hor2d) );
}


EMSeedPicker*  Horizon2DTracker::getSeedPicker( bool createnew )
{
    if ( seedpicker_ )
	return seedpicker_;

    if ( !createnew )
	return nullptr;

    seedpicker_ = new Horizon2DSeedPicker( *this );
    return seedpicker_;
}

} // namespace MPE
