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
#include "mpeengine.h"
#include "undo.h"

namespace MPE {

class PatchUndoEvent: public UndoEvent
{
public:
		    PatchUndoEvent( Patch* patch,
			const TrcKeyValue& trkv, int index )
			: patch_( patch )
			, trkv_( trkv )
			, index_( index )
		    {}

    const char* getStandardDesc() const
    { return "Horizon 3d patch line data"; }


    bool reDo()
    {
	if ( !patch_ ) return false;
	patch_->addSeed( trkv_ );
	return true;
    }


    bool unDo()
    {
	if ( !patch_ ) return false;
	patch_->removeSeed( index_ );
	return true;
    }

private:
    RefMan<Patch> patch_;
    TrcKeyValue trkv_;
    int		index_;
};



Patch::Patch( const EMSeedPicker* seedpicker )
    : seedpicker_( seedpicker )
{}


Patch::~Patch() {}


const TypeSet<TrcKeyValue>&  Patch::getPath() const
{ return seeds_; }


int Patch::nrSeeds()
{ return seeds_.size(); }


EM::PosID Patch::seedNode( int idx ) const
{
    const EM::EMObject* emobj = seedpicker_->emTracker().emObject();
    if ( idx>=seeds_.size() || !emobj )
	return EM::PosID::udf();

    const TrcKey tck = seeds_[idx].tk_;

    return EM::PosID( emobj->id(), emobj->sectionID(0), tck.pos().toInt64() );
}


Coord3 Patch::seedCoord( int idx ) const
{
    const EM::EMObject* emobj = seedpicker_->emTracker().emObject();
    if ( idx>=seeds_.size() || !emobj )
	return Coord3::udf();

    const TrcKey tck = seeds_[idx].tk_;
    const EM::PosID pid =
	EM::PosID( emobj->id(), emobj->sectionID(0), tck.pos().toInt64() );

    Coord3 pos = emobj->getPos( pid );
    pos.z = seeds_[idx].val_;

    return pos;
}


int Patch::addSeed( const TrcKeyValue& tckv )
{
    const EM::EMObject* emobj = seedpicker_->emTracker().emObject();

    BinID dir;
    if ( !emobj ||
	 !tckv.isDefined() ||
	 seeds_.indexOf(tckv) !=-1 ||
	 !seedpicker_->lineTrackDirection(dir) )
	return -1;

    if ( seeds_.size()==0 )
    {
	seeds_ += tckv;
	return seeds_.size()-1;
    }

    const bool crdir = dir.col()>0;
    const EM::PosID pid =
	EM::PosID( emobj->id(),emobj->sectionID(0),tckv.tk_.pos().toInt64() );

    int idx = findClosedSeed( pid );
    if ( tckv.tk_==seeds_[idx].tk_ )
	return -1;

    const EM::PosID& pos = seedNode( idx );
    const int curval=crdir ? pid.getRowCol().col() : pid.getRowCol().row();
    const int posval=crdir ? pos.getRowCol().col() : pos.getRowCol().row();

    if ( curval>=posval && seeds_.size()<curval )
	idx++;

    seeds_.insert( idx, tckv );
    return idx;
}


void Patch::removeSeed( int idx )
{
    if ( seeds_.size() > idx )
	seeds_.removeSingle( idx );

}



#define nMaxInteger 2109876543
int Patch::findClosedSeed( const EM::PosID& pid )
{
    BinID dir;
    if ( seeds_.size()==0 || !seedpicker_->lineTrackDirection(dir) )
	return 0;

    const bool crdir = dir.col()>0;
    int mindiff = nMaxInteger;
    int minidx = 0;
    for ( int idx=0; idx<seeds_.size(); idx++ )
    {
	const EM::PosID& pos = seedNode( idx );
	const int curval=crdir ? pid.getRowCol().col() : pid.getRowCol().row();
	const int posval=crdir ? pos.getRowCol().col() : pos.getRowCol().row();
	if ( abs(curval-posval)< mindiff )
	{
	    mindiff = abs( curval-posval );
	    minidx = idx;
	}
    }
    return minidx;

}


void Patch::clear()
{ seeds_.erase(); }


EMSeedPicker::EMSeedPicker( EMTracker& tracker )
    : tracker_(tracker)
    , seedAdded(this)
    , seedRemoved(this)
    , seedToBeAddedRemoved(this)
    , blockpicking_(false)
    , didchecksupport_(false)
    , trackmode_(TrackFromSeeds)
    , selspec_(0)
    , sowermode_(false)
    , patch_(0)
    , patchundo_( *new Undo() )
{
    sectionid_ = tracker.emObject() ? tracker.emObject()->sectionID(0) : -1;
}


EMSeedPicker::~EMSeedPicker()
{
    if ( patch_ )
	patch_->unRef();

    patchundo_.removeAll();
    delete &patchundo_;
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
{
    if ( patch_ && patch_->nrSeeds()>0 )
    {
	updatePatchLine( yn );
	patch_->clear();
    }
    patchundo_.removeAll();
}


void EMSeedPicker::setSeedPickArea( const TrcKeySampling& tks )
{ seedpickarea_ = tks; }

const TrcKeySampling& EMSeedPicker::getSeedPickArea() const
{ return seedpickarea_; }

void EMSeedPicker::setTrackMode( TrackMode tm )
{ trackmode_ = tm; }

EMSeedPicker::TrackMode EMSeedPicker::getTrackMode() const
{ return trackmode_; }


const Undo& EMSeedPicker::horPatchUndo() const	{ return patchundo_; }
Undo& EMSeedPicker::horPatchUndo()		{ return patchundo_; }


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


void EMSeedPicker::addSeedToPatch( const TrcKeyValue& tckv )
{
    if ( !patch_ )
    {
	patch_ = new Patch( this );
	patch_->ref();
    }
    const int idx = patch_->addSeed( tckv );
    if ( idx <0  )
	return;
    PatchUndoEvent* undoevent = new PatchUndoEvent( patch_, tckv, idx );
    patchundo_.addEvent(undoevent,0);
    patchundo_.setUserInteractionEnd( patchundo_.currentEventID() );
}


bool EMSeedPicker::canUndo()
{  return patchundo_.canUnDo(); }

bool EMSeedPicker::canReDo()
{  return patchundo_.canReDo(); }


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


bool EMSeedPicker::lineTrackDirection( BinID& dir,
					    bool perptotrackdir ) const
{
    const TrcKeyZSampling& activevol = engine().activeVolume();
    dir = activevol.hsamp_.step_;

    const bool inltracking = activevol.nrInl()==1 && activevol.nrCrl()>1;
    const bool crltracking = activevol.nrCrl()==1 && activevol.nrInl()>1;

    if ( !inltracking && !crltracking )
	return false;

    if ( (!perptotrackdir && inltracking) || (perptotrackdir && crltracking) )
	dir.inl() = 0;
    else
	dir.crl() = 0;

    return true;
}


} // namespace MPE

