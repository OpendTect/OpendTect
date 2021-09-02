/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2002
________________________________________________________________________

-*/

#include "horizon3dtracker.h"

#include "cubicbeziercurve.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "horizonadjuster.h"
#include "horizon3dextender.h"
#include "horizon3dseedpicker.h"
#include "mpeengine.h"
#include "sectionselectorimpl.h"
#include "sectiontracker.h"
#include "survinfo.h"


#include <math.h>

namespace MPE
{

const char* Horizon3DTracker::keyword()			{ return "Horizon3D"; }

Horizon3DTracker::Horizon3DTracker( EM::Horizon3D* hor )
    : EMTracker(hor)
    , seedpicker_( 0 )
{
    setTypeStr( Horizon3DTracker::keyword() );
}


Horizon3DTracker::~Horizon3DTracker()
{
    delete seedpicker_;
}


EMTracker* Horizon3DTracker::create( EM::EMObject* emobj )
{
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)
    if ( !hor ) return 0;

    return new Horizon3DTracker( hor );
}


void Horizon3DTracker::initClass()
{ TrackerFactory().addCreator( create, EM::Horizon3D::typeStr() ); }


#define mErrRet(msg) { errmsg = msg; return false; }

SectionTracker* Horizon3DTracker::createSectionTracker( EM::SectionID sid )
{
    if ( !getHorizon() ) return 0;

    return new SectionTracker( *emObject(), sid,
		new BinIDSurfaceSourceSelector(*getHorizon(),sid),
		ExtenderFactory().create( getTypeStr(),getHorizon(),sid),
		new HorizonAdjuster(*getHorizon(),sid) );
}


EMSeedPicker* Horizon3DTracker::getSeedPicker( bool createifnotpresent )
{
    if ( seedpicker_ )
	return seedpicker_;

    if ( !createifnotpresent )
	return 0;

    seedpicker_ = new Horizon3DSeedPicker(*this);
    return seedpicker_;
}


EM::Horizon3D* Horizon3DTracker::getHorizon()
{
    mDynamicCastGet(EM::Horizon3D*,hor,emObject());
    return hor;
}


const EM::Horizon3D* Horizon3DTracker::getHorizon() const
{ return const_cast<Horizon3DTracker*>(this)->getHorizon(); }


} // namespace MPE
