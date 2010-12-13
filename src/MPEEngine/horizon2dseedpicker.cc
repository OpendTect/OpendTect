/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Sep 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: horizon2dseedpicker.cc,v 1.24 2010-12-13 07:07:43 cvssatyaki Exp $";

#include "horizon2dseedpicker.h"

#include "attribdataholder.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "seisinfo.h"
#include "sectionextender.h"
#include "horizonadjuster.h"
#include "sectiontracker.h"
#include "executor.h"
#include "mpeengine.h"
#include "sorting.h"
#include "surv2dgeom.h"


namespace MPE 
{

Horizon2DSeedPicker::Horizon2DSeedPicker( MPE::EMTracker& t )
    : tracker_( t )
    , sectionid_( -1 )
    , addrmseed_( this )
    , surfchange_( this )
    , seedconmode_( defaultSeedConMode() )
    , blockpicking_( false )
    , selspec_(0)
    , sowermode_( false )
    , lastseedpicked_( EM::PosID::udf() )
{
    mDynamicCastGet(EM::Horizon2D*,hor,EM::EMM().getObject(tracker_.objectID()))
    if ( hor && hor->nrSections()>0 )
	sectionid_ = hor->sectionID(0);
}


void Horizon2DSeedPicker::setLine( const MultiID& lineset, const char* linenm )
{
     lineset_ = lineset;
     linename_ = linenm;
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


bool Horizon2DSeedPicker::canAddSeed( const Attrib::SelSpec& as )
{
    if ( seedconmode_ == DrawBetweenSeeds )
	return true;

    addrmseed_.trigger();
    ObjectSet<const Attrib::SelSpec> neededattribs;
    tracker_.getNeededAttribs( neededattribs );
    for ( int idx=0; idx<neededattribs.size(); idx++ )
    {
	if ( neededattribs[idx]->id() == as.id() )
	    return true;
    }
    return false;
}


#define mGetHorizon(hor,escval) \
    const EM::ObjectID emobjid = tracker_.objectID(); \
    mDynamicCastGet(EM::Horizon2D*,hor,EM::EMM().getObject(emobjid)) \
    if ( !hor ) return escval;


bool Horizon2DSeedPicker::startSeedPick()
{
    mGetHorizon(hor,false);
    didchecksupport_ = hor->enableGeometryChecks( false );

    PtrMan<IOObj> ioobj = IOM().get( lineset_ );
    if ( !ioobj )
	return false;

    EM::Horizon2DGeometry& geom = hor->geometry();
    for ( int idx=0; idx<geom.nrLines(); idx++ )
    {
	const PosInfo::GeomID& geomid =
	    S2DPOS().getGeomID( ioobj->name(), linename_ );
	if ( geomid == geom.lineGeomID(idx) )
	{
	    geomid_ = geomid;
	    return true;
	}
    }

    PosInfo::GeomID geomid = S2DPOS().getGeomID( ioobj->name(), linename_ );
    if ( !geomid.isOK() )
	return false;

    geom.addLine( geomid );
    geomid_ = geomid;
    return true;
}


#define mGetHorAndColrg(hor,colrg,escval) \
    mGetHorizon(hor,escval); \
    if ( sectionid_<0 || !geomid_.isOK() ) \
    	return escval; \
    const StepInterval<int> colrg = \
    	hor->geometry().colRange( sectionid_, geomid_ ); 
	
bool Horizon2DSeedPicker::addSeed(const Coord3& seedcrd, bool drop )
{
    addrmseed_.trigger();

    if ( blockpicking_ )
	return true;

    mGetHorAndColrg(hor,colrg,false);

    if ( colrg.start==colrg.stop )
	return false;

    float maxdist = mUdf(float);
    int   closestcol;
    int col = 0;
    for ( col=colrg.start; col<=colrg.stop; col+=colrg.step )
    {
	const Coord coord = hor->getPos( sectionid_, geomid_, col );
	if ( !coord.isDefined() )
	    continue;

	double sqdist = coord.sqDistTo( seedcrd );
	if ( sqdist<maxdist )
	{
	    closestcol = col;
	    maxdist = sqdist;
	}
    }

    col = closestcol;
    RowCol rc( hor->geometry().lineIndex(geomid_), col );
	
    const EM::PosID pid( hor->id(), sectionid_, rc.toInt64() );

    if ( sowermode_ )
	eraseInBetween( pid, lastseedpicked_ );

    Coord3 newpos = hor->getPos( pid );
    newpos.z = seedcrd.z;
    seedlist_.erase();
    seedlist_ += pid;

    const bool pickedposwasdef = hor->isDefined( pid );
    if ( !drop || !pickedposwasdef )
    {
	hor->setPos( pid, newpos, true );
	if ( seedconmode_ != DrawBetweenSeeds )
	    tracker_.snapPositions( seedlist_ );
    }

    hor->setPosAttrib( pid, EM::EMObject::sSeedNode(), true );

    const bool res = drop ? true : retrackOnActiveLine(rc.col,pickedposwasdef);

    surfchange_.trigger();
    lastseedpicked_ = pid;
    return res;
}


int Horizon2DSeedPicker::nrLineNeighbors( int colnr ) const
{
    mGetHorAndColrg(hor,colrg,-1);
    int col = 0; 
    int nrneighbors = 0;

    for ( int idx=-1; idx<=1; idx+=2 )
    {
	col = colnr;
	while ( col>colrg.start && col<colrg.stop )
	{
	    col += idx*colrg.step;
	    const Coord3 pos = hor->getPos( sectionid_, geomid_, col );
	    if ( Coord(pos).isDefined() )
	    {	
		if ( pos.isDefined() )
		    nrneighbors++;
		break;
	    }
	}
    }
    return nrneighbors;
}


bool Horizon2DSeedPicker::removeSeed( const EM::PosID& pid, bool environment,
				      bool retrack )
{ 
    addrmseed_.trigger();

    if ( blockpicking_ )
	return true;

    mGetHorizon(hor,false);
    RowCol rc;
    rc.fromInt64( pid.subID() );
    if ( rc.row != hor->geometry().lineIndex(geomid_) )
	return false;

    hor->setPosAttrib( pid, EM::EMObject::sSeedNode(), false );
    if ( environment || !nrLineNeighbors(rc.col) )
	hor->unSetPos( pid, true );

    seedlist_.erase();
    const bool res = environment ? 
		     retrackOnActiveLine(rc.col,true,!retrack) : true;

    surfchange_.trigger();
    return res;
}


void Horizon2DSeedPicker::eraseInBetween( const EM::PosID& pid1,
					  const EM::PosID& pid2 )
{
    mGetHorAndColrg(hor,colrg,);

    EM::EMObject* emobj = EM::EMM().getObject( tracker_.objectID() );
    if ( !emobj || pid1.isUdf() || pid1.objectID()!=pid2.objectID() ||
		   pid2.isUdf() || pid1.sectionID()!=pid2.sectionID() )
	return;

    RowCol rc1, rc2, tmp;
    rc1.fromInt64( pid1.subID() );
    rc2.fromInt64( pid2.subID() );
    if ( rc1.row != rc2.row )
	return;

    if ( rc1.col > rc2.col )
	mSWAP( rc1, rc2, tmp );

    rc1.col += colrg.step;
    while ( rc1.col < rc2.col )
    {
	EM::PosID pid( pid1.objectID(), pid1.sectionID(), rc1.toInt64() );
	emobj->unSetPos( pid, true );
	emobj->setPosAttrib( pid, EM::EMObject::sSeedNode(), false );
	rc1.col += colrg.step;
    }
}


bool Horizon2DSeedPicker::retrackOnActiveLine( int startcol, 
					       bool startwasdefined,
					       bool eraseonly ) 
{
    mGetHorAndColrg(hor,colrg,false);
    
    trackbounds_.erase();   
    junctions_.erase();

    if ( colrg.includes(startcol) )
    {
	extendSeedListEraseInBetween( false, startcol, startwasdefined,
				      -colrg.step );
	extendSeedListEraseInBetween( false, startcol, startwasdefined,
				      colrg.step );
    }
    else
    {
	// traverse whole active line
	extendSeedListEraseInBetween( true, colrg.start, false, -colrg.step );
	extendSeedListEraseInBetween( true, colrg.start-colrg.step, false,
				      colrg.step );

	if ( seedconmode_ != DrawBetweenSeeds )
	    tracker_.snapPositions( seedlist_ );
    }

    bool res = true;

    if ( eraseonly )
	return true;

    res = retrackFromSeedList();

    for ( int idx=0; idx<junctions_.size(); idx+=2 )
    {
	if ( hor->isDefined(junctions_[idx]) )
	    hor->setPosAttrib(junctions_[idx+1], 
		    	      EM::EMObject::sSeedNode(), true);
    }

    return res;
}


void Horizon2DSeedPicker::extendSeedListEraseInBetween(
					    bool wholeline, int startcol, 
					    bool startwasdefined, int step )
{
    mGetHorAndColrg(hor,colrg, );
    eraselist_.erase();

    RowCol currc( hor->geometry().lineIndex(geomid_), startcol );
    EM::PosID curpid = EM::PosID( hor->id(), sectionid_, currc.toInt64());

    bool seedwasadded = hor->isDefined( curpid ) && !wholeline;
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

	    if ( seedwasadded && seedconmode_!=TrackFromSeeds )
		return;

	    break;
	}
	
	const EM::PosID pid( hor->id(), sectionid_, currc.toInt64() );

	// Skip if survey coordinates undefined
	if ( !Coord(hor->getPos(pid)).isDefined() )
	    continue;

	curpid = pid;
	curdefined = hor->isDefined(curpid);
	
	// running into a seed point
	if ( hor->isPosAttrib( curpid, EM::EMObject::sSeedNode() ) )
	{
	    const bool onewaytracking = seedwasadded && !prevdefined &&
					seedconmode_==TrackFromSeeds;
	    if ( onewaytracking )
		trackbounds_ += curpid;
	    else
		seedlist_ += curpid;
	    
	    if ( wholeline )
		continue;

	    break;
	}

	// running into a loose end
	if ( !wholeline && !prevdefined && curdefined )
	{
	    if  ( seedconmode_==TrackFromSeeds )
		trackbounds_ += curpid;
	    else
		seedlist_ += curpid;

	    junctions_ += prevpid;
	    junctions_ += curpid;
	    break;
	}

	// to erase points attached to start
	if ( curdefined )
	    eraselist_ += curpid;
    }

    for ( int idx=0; idx<eraselist_.size(); idx++ )
	hor->unSetPos( eraselist_[idx], true );
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

	for ( int idx=addedpos.size()-1; idx>=0; idx-- )
	{
	    if ( !hor->isDefined(sectionid_,addedpos[idx]) )
		addedpos.remove(idx);
	}
    }

    extender->unsetExtBoundary();

    return true;
}


bool Horizon2DSeedPicker::doesModeUseSetup() const
{ return seedconmode_ != DrawBetweenSeeds; }


bool Horizon2DSeedPicker::reTrack()
{ 
    seedlist_.erase();
    const bool res = retrackOnActiveLine( Values::Undef<int>::val() , false );
    surfchange_.trigger();
    return res;
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
			      hor->getPosAttribList( EM::EMObject::sSeedNode() );
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

    mAllocVarLenArr( int, sortval, nrseeds );
    mAllocVarLenArr( int, sortidx, nrseeds );

    RowCol rc;
    for ( int idx=0; idx<nrseeds; idx++ )
    {
	rc.fromInt64( seedlist_[idx].subID() );
	if ( rc.row != hor->geometry().lineIndex(geomid_) )
	    return false;

	sortval[idx] = rc.col;
	sortidx[idx] = idx;
    }

    sort_coupled( mVarLenArr(sortval), mVarLenArr(sortidx), nrseeds );

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
	    const Coord curpos = hor->getPos( sectionid_, rc.toInt64() );
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
	    const Coord curpos = hor->getPos( sectionid_, rc.toInt64() );
	    if ( !curpos.isDefined() ) 
		continue;
	    arclen += prevpos.distTo( curpos );
	    prevpos = curpos;
	    
	    const double frac = arclen / totarclen;
	    const double curz = (1-frac) * startpos.z + frac * endpos.z;
	    const Coord3 interpos( curpos, curz ); 
	    hor->setPos( sectionid_, rc.toInt64(), interpos, true );
	}
    }
    return true;
}

#define mAddToBox(pidlist) \
    for ( int idx=0; idx<pidlist.size(); idx++ ) \
    { \
	rc.fromInt64( pidlist[idx].subID() ); \
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

