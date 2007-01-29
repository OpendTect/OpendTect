/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Sep 2005
___________________________________________________________________

-*/

static const char* rcsID = "$Id: horizon2dseedpicker.cc,v 1.4 2007-01-29 21:02:50 cvskris Exp $";

#include "horizon2dseedpicker.h"

#include "attribdataholder.h"
//#include "autotracker.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "seisinfo.h"
#include "sectionextender.h"
#include "horizonadjuster.h"
#include "sectiontracker.h"
#include "executor.h"
#include "mpeengine.h"
#include "sorting.h"


namespace MPE 
{

Horizon2DSeedPicker::Horizon2DSeedPicker( MPE::EMTracker& t )
    : tracker_( t )
    , data2d_( 0 )
    , sectionid_( -1 )
    , lineid_( -1 )
    , addrmseed_( this )
    , surfchange_( this )
    , seedconmode_( defaultSeedConMode() )
    , blockpicking_( false )
{ }


Horizon2DSeedPicker::~Horizon2DSeedPicker()
{
    if ( data2d_ ) data2d_->unRef();
}


void Horizon2DSeedPicker::setLine( const MultiID& lineset, const char* linename,
				   const Attrib::Data2DHolder* data )
{
     if ( data2d_ )
	 data2d_->unRef();

     data2d_ = data;
     if ( data2d_ ) data2d_->ref();

     lineset_ = lineset;
     linename_ = linename;
}


bool Horizon2DSeedPicker::canSetSectionID() const
{ return true; }


bool Horizon2DSeedPicker::setSectionID( const EM::SectionID& sid )
{ 
    sectionid_ = sid; 
    return true; 
}


EM::SectionID Horizon2DSeedPicker::getSectionID() const
{ return sectionid_; }


#define mGetHorizon(hor,escval) \
    const EM::ObjectID emobjid = tracker_.objectID(); \
    mDynamicCastGet( EM::Horizon2D*, hor, EM::EMM().getObject(emobjid) ); \
    if ( !hor ) \
	return escval;


bool Horizon2DSeedPicker::startSeedPick()
{
    if ( !data2d_ ) return false;

    mGetHorizon(hor,false);
    didchecksupport_ = hor->enableGeometryChecks( false );

    bool found = false;
    for ( int idx=0; idx<hor->geometry().nrLines(); idx++ )
    {
	const int lineid = hor->geometry().lineID( idx );
	if ( hor->geometry().lineSet(lineid)==lineset_ &&
	     !strcmp(hor->geometry().lineName(lineid),linename_) )
	{
	    found = true;
	    lineid_ = lineid;
	    break;
	}
    }

    if ( !found )
    {
	TypeSet<Coord> coords;
	const CubeSampling cs = data2d_->getCubeSampling();
	const StepInterval<int> trcrg( cs.hrg.start.crl, cs.hrg.stop.crl,
				       cs.hrg.step.crl );

	for ( int trcnr=trcrg.start; trcnr<=trcrg.stop; trcnr+= trcrg.step )
	{
	    found = false;
	    for ( int idx=0; idx<data2d_->size(); idx++ )
	    {
		if ( data2d_->trcinfoset_[idx]->nr == trcnr )
		{
		    coords += data2d_->trcinfoset_[idx]->coord;
		    found = true;
		    break;
		}
	    }

	    if ( !found ) coords += Coord::udf();
	}

	lineid_ = hor->geometry().addLine( coords, trcrg.start, trcrg.step,
					   lineset_, linename_ );
    }

    return true;
}


#define mGetHorAndColrg(hor,colrg,escval) \
    mGetHorizon(hor,escval); \
    if ( sectionid_<0 || lineid_<0 ) \
    	return escval; \
    const StepInterval<int> colrg = \
    	hor->geometry().colRange( sectionid_, lineid_ ); 
	
bool Horizon2DSeedPicker::addSeed(const Coord3& seedcrd )
{
    addrmseed_.trigger();

    if ( blockpicking_ )
	return true;

    mGetHorAndColrg(hor,colrg,false);

    if ( colrg.start==colrg.stop )
	return false;

    float maxdist = mUdf(float);
    int   closestcol;
    RowCol rc( lineid_, 0 );
    for ( rc.col=colrg.start; rc.col<=colrg.stop; rc.col+=colrg.step )
    {
	const Coord coord = hor->getPos( sectionid_, rc.getSerialized() );
	if ( !coord.isDefined() )
	    continue;

	double sqdist = coord.sqDistTo( seedcrd );
	if ( sqdist<maxdist )
	{
	    closestcol = rc.col;
	    maxdist = sqdist;
	}
    }

    rc.col = closestcol;
	
    const EM::PosID pid( hor->id(), sectionid_, rc.getSerialized() );

    const bool startwasdefined = hor->isDefined( pid );

    hor->setPos( pid, seedcrd, true );
    if ( !hor->isPosAttrib( pid, EM::EMObject::sSeedNode ) )
	hor->setPosAttrib( pid, EM::EMObject::sSeedNode, true );

    seedlist_.erase();
    seedlist_ += pid;

    if ( seedconmode_ != DrawBetweenSeeds )
	tracker_.snapPositions( seedlist_ );

    return retrackOnActiveLine( rc.col, startwasdefined );
}


bool Horizon2DSeedPicker::removeSeed( const EM::PosID& pid, bool retrack )
{ 
    addrmseed_.trigger();

    if ( blockpicking_ )
	return true;

    mGetHorizon(hor,false);
    hor->unSetPos( pid, true );
    seedlist_.erase();

    RowCol rc;
    rc.setSerialized( pid.subID() );
    if ( rc.row != lineid_ )
	return false;

    return retrackOnActiveLine( rc.col, true, !retrack );
}


bool Horizon2DSeedPicker::retrackOnActiveLine( int startcol, 
					       bool startwasdefined,
					       bool eraseonly ) 
{
    mGetHorAndColrg(hor,colrg,false);
    
    trackbounds_.erase();   
    TypeSet<EM::PosID> candidatejunctionpairs;

    if ( colrg.includes(startcol) )
    {
	extendSeedListEraseInBetween( false, startcol, startwasdefined,
				      -colrg.step, candidatejunctionpairs );
	extendSeedListEraseInBetween( false, startcol, startwasdefined,
				      colrg.step, candidatejunctionpairs );
    }
    else
    {
	// traverse whole active line
	extendSeedListEraseInBetween( true, colrg.start, false,
				      -colrg.step, candidatejunctionpairs );
	extendSeedListEraseInBetween( true, colrg.start-colrg.step, false,
				      colrg.step, candidatejunctionpairs );

	if ( seedconmode_ != DrawBetweenSeeds )
	    tracker_.snapPositions( seedlist_ );
    }

    bool res = true;

    if ( !eraseonly )
    {
	res = retrackFromSeedList();

	for ( int idx=0; idx<candidatejunctionpairs.size(); idx+=2 )
	{
	    if ( hor->isDefined(candidatejunctionpairs[idx]) )
	    {
		hor->setPosAttrib( candidatejunctionpairs[idx+1],
				   EM::EMObject::sSeedNode, true );
	    }
	}
    }

    surfchange_.trigger();
    return res;
}


void Horizon2DSeedPicker::extendSeedListEraseInBetween(
			bool wholeline, int startcol, bool startwasdefined,
			int step, TypeSet<EM::PosID>& candidatejunctionpairs )
{
    mGetHorAndColrg(hor,colrg, );

    RowCol currc( lineid_, startcol );
    EM::PosID curpid = EM::PosID( hor->id(), sectionid_, currc.getSerialized());
    bool curdefined = startwasdefined;
    
    while ( true )
    {
	const EM::PosID prevpid = curpid;
	const bool prevdefined = curdefined;

	currc.col += step;

	// reaching end of line
	if ( !colrg.includes(currc.col) )
	{
	    if  ( seedconmode_ == TrackFromSeeds )
		trackbounds_ += prevpid;
	    return;
	}
	
	const EM::PosID pid( hor->id(), sectionid_, currc.getSerialized() );

	// Skip if survey coordinates undefined
	if ( !Coord(hor->getPos(pid)).isDefined() )
	    continue;

	curpid = pid;
	curdefined = hor->isDefined(curpid);
	
	// running into a seed point
	if ( hor->isPosAttrib( curpid, EM::EMObject::sSeedNode ) )
	{
	    seedlist_ += curpid;
	    if ( !wholeline )
		return;

	    continue;
	}

	// running into a loose end
	if ( !wholeline && !prevdefined && curdefined )
	{
	    if  ( seedconmode_==DrawBetweenSeeds )
		seedlist_ += curpid;
	    else
		trackbounds_ += curpid;

	    candidatejunctionpairs += prevpid;
	    candidatejunctionpairs += curpid;
	    return;
	}

	// erasing points attached to start
	if ( curdefined )
	    hor->unSetPos( curpid, true );
    }
    return;
}


bool Horizon2DSeedPicker::retrackFromSeedList()
{
    if ( seedlist_.isEmpty() )
	return true;
    if ( blockpicking_ )
	return true;
    if ( seedconmode_ == DrawBetweenSeeds )
	return interpolateSeeds();

    mGetHorizon(hor,false);

    SectionTracker* sectracker = tracker_.getSectionTracker( sectionid_, true );
    SectionExtender* extender = sectracker->extender();
    mDynamicCastGet( HorizonAdjuster*, adjuster, sectracker->adjuster() );
    
    extender->setDirection( BinIDValue(BinID(0,0), mUdf(float)) );
    extender->setExtBoundary( getTrackBox() );

    TypeSet<EM::SubID> addedpos;
    TypeSet<EM::SubID> addedpossrc;

    for ( int idx=0; idx<seedlist_.size(); idx++ )
	addedpos += seedlist_[idx].subID();

    while ( addedpos.size() )
    {
	extender->reset();
	extender->setStartPositions( addedpos );
	while ( extender->nextStep()>0 );

	addedpos = extender->getAddedPositions();
	addedpossrc = extender->getAddedPositionsSource();

	adjuster->reset();
	adjuster->setPositions(addedpos, &addedpossrc);
	while ( adjuster->nextStep()>0 );

	for ( int idx=0; idx<addedpos.size(); idx++ )
	{
	    if ( !hor->isDefined(sectionid_,addedpos[idx]) )
		addedpos.remove(idx);
	}
    }

    extender->unsetExtBoundary();
    
    return true;
}
	    

bool Horizon2DSeedPicker::doesModeUseSetup() const
{ return seedconmode_!=DrawBetweenSeeds; }


bool Horizon2DSeedPicker::reTrack()
{ 
    seedlist_.erase();
    return retrackOnActiveLine( Values::Undef<int>::val() , false );
}


int Horizon2DSeedPicker::getSeedConnectMode() const
{ return seedconmode_; }


void Horizon2DSeedPicker::blockSeedPick( bool yn )
{ blockpicking_ = yn; }


int Horizon2DSeedPicker::minSeedsToLeaveInitStage() const
{
    if ( seedconmode_==TrackFromSeeds )
	return 1;
    else if ( seedconmode_==TrackBetweenSeeds )
	return 2;
    else
	return 0 ;
}


void Horizon2DSeedPicker::setSeedConnectMode( int scm )
{ seedconmode_ = scm; }


bool Horizon2DSeedPicker::isSeedPickBlocked() const
{ return blockpicking_; }


bool Horizon2DSeedPicker::doesModeUseVolume() const
{ return false; }


bool Horizon2DSeedPicker::stopSeedPick(bool)
{
    mGetHorizon(hor,false);
    hor->enableGeometryChecks( didchecksupport_ );
    return true;
}


int MPE::Horizon2DSeedPicker::nrSeeds() const
{
    mGetHorizon(hor,false);
    const TypeSet<EM::PosID>* seednodelist = 
			      hor->getPosAttribList( EM::EMObject::sSeedNode );
    return seednodelist ? seednodelist->size() : 0;
}


const char* Horizon2DSeedPicker::seedConModeText( int mode, bool abbrev )
{
    if ( mode==TrackFromSeeds && !abbrev )
	return "Track from seed(s)";
    else if ( mode==TrackFromSeeds && abbrev )
	return "Track from";
    else if ( mode==TrackBetweenSeeds && !abbrev )
	return "Track between seeds";
    else if ( mode==TrackBetweenSeeds && abbrev )
	return "Track between";
    else if ( mode==DrawBetweenSeeds && !abbrev )
	return "Draw between seeds";
    else if ( mode==DrawBetweenSeeds && abbrev )
	return "Draw between";
    else
	return "Unknown mode";
}


int Horizon2DSeedPicker::defaultSeedConMode( bool gotsetup ) const
{ return gotsetup ? defaultSeedConMode() : DrawBetweenSeeds; }


bool Horizon2DSeedPicker::interpolateSeeds()
{
    mGetHorAndColrg(hor,colrg,false);

    const int nrseeds = seedlist_.size();
    if ( nrseeds<2 )
	return true;

    int sortval[nrseeds], sortidx[nrseeds];
    
    RowCol rc;
    for ( int idx=0; idx<nrseeds; idx++ )
    {
	rc.setSerialized( seedlist_[idx].subID() );
	if ( rc.row != lineid_ )
	    return false;

	sortval[idx] = rc.col;
	sortidx[idx] = idx;
    }
    sort_coupled( sortval, sortidx, nrseeds );

    for ( int vtx=0; vtx<nrseeds-1; vtx++ )
    {
	const Coord3 startpos = hor->getPos( seedlist_[ sortidx[vtx] ] );
	const Coord3 endpos = hor->getPos( seedlist_[ sortidx[vtx+1] ] );
	
	double totarclen = 0.0;
	Coord prevpos = startpos;
	rc.col = sortval[vtx];
	while ( rc.col<sortval[vtx+1] )
	{
	    rc.col += colrg.step;
	    const Coord curpos = hor->getPos( sectionid_, rc.getSerialized() );
	    if ( !curpos.isDefined() ) 
		continue;
	    totarclen += prevpos.distTo( curpos );
	    prevpos = curpos;
	}

	double arclen = 0.0;
	prevpos = startpos;
	rc.col = sortval[vtx] + colrg.step;
	for ( ; rc.col<sortval[vtx+1]; rc.col += colrg.step  )
	{
	    const Coord curpos = hor->getPos( sectionid_, rc.getSerialized() );
	    if ( !curpos.isDefined() ) 
		continue;
	    arclen += prevpos.distTo( curpos );
	    prevpos = curpos;
	    
	    const double frac = arclen / totarclen;
	    const double curz = (1-frac) * startpos.z + frac * endpos.z;
	    const Coord3 interpos( curpos, curz ); 
	    hor->setPos( sectionid_, rc.getSerialized(), interpos, true );
	}
    }
    return true;
}

#define mAddToBox(pidlist) \
    for ( int idx=0; idx<pidlist.size(); idx++ ) \
    { \
	rc.setSerialized( pidlist[idx].subID() ); \
	trackbox.hrg.include( BinID(rc) ); \
    }

CubeSampling Horizon2DSeedPicker::getTrackBox() const
{
    CubeSampling trackbox( true );
    trackbox.hrg.init( false );
    RowCol rc;
    mAddToBox(seedlist_);
    mAddToBox(trackbounds_);

    return trackbox;
}


}; // namespace MPE

