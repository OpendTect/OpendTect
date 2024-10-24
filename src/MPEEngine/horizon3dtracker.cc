/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizon3dtracker.h"

#include "autotracker.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "horizonadjuster.h"
#include "horizon3dextender.h"
#include "horizon3dseedpicker.h"
#include "sectionselectorimpl.h"
#include "sectiontracker.h"


RefMan<MPE::Horizon3DTracker>
			MPE::Horizon3DTracker::create( EM::Horizon3D& hor )
{
    return new Horizon3DTracker( hor );
}


MPE::Horizon3DTracker::Horizon3DTracker( EM::Horizon3D& hor )
    : EMTracker(hor)
{
    setTypeStr( EM::Horizon3D::typeStr() );
}


MPE::Horizon3DTracker::~Horizon3DTracker()
{
    detachAllNotifiers();
    delete seedpicker_;
    delete htmgr_;
}


bool MPE::Horizon3DTracker::hasTrackingMgr() const
{
    return htmgr_;
}


bool MPE::Horizon3DTracker::createMgr()
{
    if ( htmgr_ )
    {
	pErrMsg("Wrong usage: There is already a tracking manager");
	return true;
    }

    htmgr_ = new HorizonTrackerMgr( *this );
    mAttachCB( htmgr_->finished, Horizon3DTracker::trackingFinishedCB );
    return true;
}


void MPE::Horizon3DTracker::startFromSeeds( const TypeSet<TrcKey>& seeds )
{
    if ( !htmgr_ )
	createMgr();

    htmgr_->startFromSeeds( seeds );
}


void MPE::Horizon3DTracker::initTrackingMgr()
{
    if ( htmgr_ )
	htmgr_->init();
}


bool MPE::Horizon3DTracker::trackingInProgress() const
{
    return htmgr_ ? htmgr_->hasTasks() : false;
}


void MPE::Horizon3DTracker::updateFlatCubesContainer(
				const TrcKeyZSampling& tkzs, bool addremove )
{
    if ( !htmgr_ )
	createMgr();

    htmgr_->updateFlatCubesContainer( tkzs, addremove );
}


void MPE::Horizon3DTracker::stopTracking()
{
    if ( htmgr_ )
	htmgr_->stop();
}


#define mErrRet(msg) { errmsg = msg; return false; }

MPE::SectionTracker* MPE::Horizon3DTracker::createSectionTracker()
{
    RefMan<EM::EMObject> emobject = emObject();
    mDynamicCastGet(EM::Horizon3D*,hor3d,emobject.ptr());
    if ( !hor3d )
	return nullptr;

    Horizon3DExtenderBase* extender3d =
			    Horizon3DExtenderBase::createInstance( *hor3d );

    EM::EMObject* emobj = emobject.ptr();
    return new SectionTracker( *emobj, new BinIDSurfaceSourceSelector(*hor3d),
			       extender3d, new HorizonAdjuster(*hor3d) );
}


MPE::EMSeedPicker* MPE::Horizon3DTracker::getSeedPicker(
						bool createifnotpresent )
{
    if ( seedpicker_ )
	return seedpicker_;

    if ( !createifnotpresent )
	return nullptr;

    seedpicker_ = new Horizon3DSeedPicker( *this );
    return seedpicker_;
}
