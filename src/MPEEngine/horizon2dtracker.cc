/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "horizon2dtracker.h"

#include "cubicbeziercurve.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "horizonadjuster.h"
#include "horizon2dextender.h"
#include "horizon2dseedpicker.h"
#include "horizon2dselector.h"
#include "mpeengine.h"
#include "sectiontracker.h"
#include "survinfo.h"


#include <math.h>

namespace MPE
{

const char* Horizon2DTracker::keyword()			{ return "Horizon2D"; }

Horizon2DTracker::Horizon2DTracker( EM::Horizon2D* hor )
    : EMTracker(hor)
    , seedpicker_( 0 )
{
    setTypeStr( Horizon2DTracker::keyword() );
}


Horizon2DTracker::~Horizon2DTracker()
{
//    delete seedpicker_;
}


EMTracker* Horizon2DTracker::create( EM::EMObject* emobj )
{
    mDynamicCastGet(EM::Horizon2D*,hor,emobj)
    return emobj && !hor ? 0 : new Horizon2DTracker( hor );
}


void Horizon2DTracker::initClass()
{
    TrackerFactory().addCreator( create, EM::Horizon2D::typeStr() );
}


#define mErrRet(msg) { errmsg = msg; return false; }

SectionTracker* Horizon2DTracker::createSectionTracker( EM::SectionID sid )
{
    if ( !getHorizon2D() ) return 0;

    return new SectionTracker( *emObject(), sid,
	    new Horizon2DSelector(*getHorizon2D(),sid),
//	    new Horizon2DExtender(*getHorizon2D(),sid),
	    ExtenderFactory().create( getTypeStr(),getHorizon2D(),sid),
	    new HorizonAdjuster(*getHorizon2D(),sid) );
}


EMSeedPicker*  Horizon2DTracker::getSeedPicker(bool createifnotpresent)
{
    if ( seedpicker_ )
	return seedpicker_;

    if ( !createifnotpresent )
	return 0;

    seedpicker_ = new Horizon2DSeedPicker(*this);
    return seedpicker_;
}


EM::Horizon2D* Horizon2DTracker::getHorizon2D()
{
    mDynamicCastGet(EM::Horizon2D*,hor,emObject());
    return hor;
}


const EM::Horizon2D* Horizon2DTracker::getHorizon2D() const 
{ return const_cast<Horizon2DTracker*>(this)->getHorizon2D(); }


}; // namespace MPE
