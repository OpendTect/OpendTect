/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: faulttracker.cc,v 1.3 2007-09-13 06:05:29 cvskris Exp $";

#include "faulttracker.h"

#include "cubicbeziercurve.h"
#include "emfault.h"
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
    mDynamicCastGet( EM::Fault*, fault, emo );
    if ( emo && !fault )
	return 0;

    return new FaultTracker( fault );
}


void FaultTracker::initClass()
{ MPE::TrackerFactory().addCreator( create, EM::Fault::typeStr() ); }


FaultTracker::FaultTracker( EM::Fault* fault_ )
    : EMTracker( fault_ )
    , seedpicker( 0 )
{ }


FaultTracker::~FaultTracker()
{
    delete seedpicker;
}


SectionTracker* FaultTracker::createSectionTracker( EM::SectionID sid )
{
    if ( !getFault() ) return 0;

    SurfaceSourceSelector* srcsel =
	new SurfaceSourceSelector( *getFault(), sid );

    FaultExtender* extender =
	new FaultExtender( *getFault(), sid );
    extender->setMaxDistance( Coord3(100,100,SI().zStep()*10) );

    FaultAdjuster* fltadj = 
	new FaultAdjuster( *getFault(), sid );

    return new SectionTracker( *emObject(), sid, srcsel, extender, fltadj );
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


EM::Fault* FaultTracker::getFault()
{
    mDynamicCastGet( EM::Fault*, fault, emObject() );
    return fault;
}


const EM::Fault* FaultTracker::getFault() const 
{ return const_cast<FaultTracker*>(this)->getFault(); }


};  //namespace
