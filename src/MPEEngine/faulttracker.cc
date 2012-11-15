/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "faulttracker.h"

#include "cubicbeziercurve.h"
#include "emfault3d.h"
#include "emmanager.h"
#include "faultseedpicker.h"
#include "mpeengine.h"
#include "faultextender.h"
#include "sectionselectorimpl.h"
#include "faultadjuster.h"
#include "sectiontracker.h"
#include "survinfo.h"



namespace MPE 
{


EMTracker* FaultTracker::create( EM::EMObject* emo )
{
    mDynamicCastGet(EM::Fault3D*,fault,emo)
    if ( emo && !fault )
	return 0;

    return new FaultTracker( fault );
}


void FaultTracker::initClass()
{ MPE::TrackerFactory().addCreator( create, EM::Fault3D::typeStr() ); }


FaultTracker::FaultTracker( EM::Fault3D* fault )
    : EMTracker( fault )
    , seedpicker( 0 )
{ }


FaultTracker::~FaultTracker()
{
    delete seedpicker;
}


SectionTracker* FaultTracker::createSectionTracker( EM::SectionID sid )
{
    return 0;
/*    if ( !getFault() ) return 0;

    SurfaceSourceSelector* srcsel =
	new SurfaceSourceSelector( *getFault(), sid );

    FaultExtender* extender =
	new FaultExtender( *getFault(), sid );
    extender->setMaxDistance( Coord3(100,100,SI().zStep()*10) );

    FaultAdjuster* fltadj = 
	new FaultAdjuster( *getFault(), sid );

    return new SectionTracker( *emObject(), sid, srcsel, extender, fltadj ); */
}


bool FaultTracker::trackIntersections( const TrackPlane& )
{ return true; }


EMSeedPicker* FaultTracker::getSeedPicker(bool createifnotpresent)
{
    if ( seedpicker )
	return seedpicker;

    if ( createifnotpresent ) seedpicker = new FaultSeedPicker(*this);

    return seedpicker;
}


EM::Fault3D* FaultTracker::getFault()
{
    mDynamicCastGet(EM::Fault3D*,fault,emObject());
    return fault;
}


const EM::Fault3D* FaultTracker::getFault() const 
{ return const_cast<FaultTracker*>(this)->getFault(); }


} // namespace MPE
