/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2015
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id: $";

#include "emseedpicker.h"

#include "emobject.h"
#include "emtracker.h"
#include "horizonadjuster.h"
#include "sectiontracker.h"


namespace MPE {

EMSeedPicker::EMSeedPicker( EMTracker& tracker )
    : tracker_(tracker)
    , seedAdded(this)
    , seedRemoved(this)
    , seedToBeAddedRemoved(this)
    , blockpicking_(false)
    , didchecksupport_(false)
    , endpatch_(false)
    , trackmode_(TrackFromSeeds)
    , selspec_(0)
    , sowermode_(false)
{
    sectionid_ = tracker.emObject() ? tracker.emObject()->sectionID(0) : -1;
}


void EMSeedPicker::setSectionID( EM::SectionID sid )
{ sectionid_ = sid; }

EM::SectionID EMSeedPicker::getSectionID() const
{ return sectionid_; }

void EMSeedPicker::setSowerMode( bool yn )
{ sowermode_ = yn; }

bool EMSeedPicker::getSowerMode() const
{ return sowermode_; }

void EMSeedPicker::blockSeedPick( bool yn )
{ blockpicking_ = yn; }

bool EMSeedPicker::isSeedPickBlocked() const
{ return blockpicking_; }

void EMSeedPicker::endPatch( bool yn )
{ endpatch_ = yn; }

bool EMSeedPicker::isPatchEnded() const
{ return endpatch_; }

void EMSeedPicker::setSeedPickArea( const TrcKeySampling& tks )
{ seedpickarea_ = tks; }

const TrcKeySampling& EMSeedPicker::getSeedPickArea() const
{ return seedpickarea_; }

int EMSeedPicker::nrTrackModes( bool )
{ return 3; }

void EMSeedPicker::setTrackMode( TrackMode tm )
{ trackmode_ = tm; }

EMSeedPicker::TrackMode EMSeedPicker::getTrackMode() const
{ return trackmode_; }


uiString EMSeedPicker::getTrackModeText( TrackMode mode, bool is2d )
{
    if ( mode==TrackFromSeeds )
	return is2d ? tr("Track from seed(s)") : tr("Track in Volume");
    else if ( mode==TrackBetweenSeeds )
	return tr("Track between Seeds");
    else if ( mode==DrawBetweenSeeds )
	return tr("Draw between Seeds");
    else
	return tr("Unknown mode");
}


void EMSeedPicker::setSelSpec( const Attrib::SelSpec* as )
{
    selspec_ = as ? *as : Attrib::SelSpec();

    SectionTracker* sectracker = tracker_.getSectionTracker( sectionid_, true );
    mDynamicCastGet(HorizonAdjuster*,adjuster,
		    sectracker?sectracker->adjuster():0);
    if ( adjuster )
	adjuster->setAttributeSel( 0, selspec_ );
}


const Attrib::SelSpec* EMSeedPicker::getSelSpec() const
{ return &selspec_; }


bool EMSeedPicker::startSeedPick()
{
    EM::EMObject* emobj = tracker_.emObject();
    if ( !emobj ) return false;

    didchecksupport_ = emobj->enableGeometryChecks( false );
    return true;
}


bool EMSeedPicker::stopSeedPick()
{
    EM::EMObject* emobj = tracker_.emObject();
    if ( !emobj ) return false;

    emobj->enableGeometryChecks( didchecksupport_ );
    return true;
}


bool EMSeedPicker::addSeed( const TrcKeyValue& seed, bool drop )
{ return addSeed( seed, drop, seed ); }


TrcKeyValue EMSeedPicker::getAddedSeed() const
{ return addedseed_; }


int EMSeedPicker::nrSeeds() const
{
    EM::EMObject* emobj = tracker_.emObject();
    if ( !emobj ) return 0;

    const TypeSet<EM::PosID>* seednodelist =
		emobj->getPosAttribList( EM::EMObject::sSeedNode() );
    return seednodelist ? seednodelist->size() : 0;
}


void EMSeedPicker::getSeeds( TypeSet<TrcKey>& seeds ) const
{
    EM::EMObject* emobj = tracker_.emObject();
    if ( !emobj ) return;

    const TypeSet<EM::PosID>* seednodelist =
			emobj->getPosAttribList( EM::EMObject::sSeedNode() );
    if ( !seednodelist ) return;

    for ( int idx=0; idx<seednodelist->size(); idx++ )
    {
	const BinID bid = BinID::fromInt64( (*seednodelist)[idx].subID() );
	seeds += TrcKey( bid );
    }
}


int EMSeedPicker::indexOf( const TrcKey& tk ) const
{
    TypeSet<TrcKey> seeds; getSeeds( seeds );
    return seeds.indexOf( tk );
}


} // namespace MPE
