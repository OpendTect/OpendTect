/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          January 2003
 RCS:           $Id: visrandomtrackdisplay.cc,v 1.32 2004-04-30 11:46:42 kristofer Exp $
 ________________________________________________________________________

-*/


#include "visrandomtrackdisplay.h"

#include "arrayndimpl.h"
#include "attribsel.h"
#include "binidselimpl.h"
#include "iopar.h"
#include "seistrc.h"
#include "interpol.h"
#include "survinfo.h"
#include "visdataman.h"
#include "vismaterial.h"
#include "visrandomtrack.h"
#include "vistristripset.h"
#include "vistransform.h"
#include "viscolortab.h"
#include "ptrman.h"

#include <math.h>


const char* visSurvey::RandomTrackDisplay::trackstr = "Random track";
const char* visSurvey::RandomTrackDisplay::nrknotsstr = "Nr. Knots";
const char* visSurvey::RandomTrackDisplay::knotprefix = "Knot ";
const char* visSurvey::RandomTrackDisplay::depthintvstr = "Depth Interval";


mCreateFactoryEntry( visSurvey::RandomTrackDisplay );

visSurvey::RandomTrackDisplay::RandomTrackDisplay()
    : VisualObject(true)
    , track(0)
    , texturematerial(visBase::Material::create())
    , as(*new AttribSelSpec)
    , colas(*new ColorAttribSel)
    , rightclick(this)
    , knotmoving(this)
    , moving(this)
    , selknotidx(-1)
    , ismanip( true )
{
    setRandomTrack( visBase::RandomTrack::create() );

    texturematerial->ref();
    texturematerial->setAmbience( 0.8 );
    texturematerial->setDiffIntensity( 0.8 );
    track->setMaterial( texturematerial );

    const StepInterval<double>& survinterval = SI().zRange(true);
    const StepInterval<float> inlrange( SI().range(true).start.inl,
	    				SI().range(true).stop.inl,
					SI().inlWorkStep() );
    const StepInterval<float> crlrange( SI().range(true).start.crl,
	    				SI().range(true).stop.crl,
	    				SI().crlWorkStep() );

    track->setDepthInterval( Interval<float>( survinterval.start,
					      survinterval.stop ));
    BinID start( mNINT(inlrange.center()), mNINT(crlrange.start) );
    BinID stop(start.inl, mNINT(crlrange.stop) );

    setKnotPos( 0, start );
    setKnotPos( 1, stop );

    track->setXrange( StepInterval<float>( inlrange.start,
		    			   inlrange.stop,
					   inlrange.step ));

    track->setYrange( StepInterval<float>( crlrange.start,
	    				   crlrange.stop,
					   crlrange.step ));

    track->setZrange( StepInterval<float>( survinterval.start,
					   survinterval.stop,
					   survinterval.step ));

    const int baselen = mNINT((inlrange.width()+crlrange.width())/2);
    
    track->setDraggerSize( Coord3( baselen/50, baselen/50,
	    			   survinterval.width()/50 ));
    showManipulator(true);
    showManipulator(false);
}


visSurvey::RandomTrackDisplay::~RandomTrackDisplay()
{
    track->knotmovement.remove( mCB(this,RandomTrackDisplay,knotMoved) );
    track->knotnrchange.remove( mCB(this,RandomTrackDisplay,knotNrChanged) );
    track->rightclick.remove( mCB(this,RandomTrackDisplay,rightClicked) );
    track->unRef();
    texturematerial->unRef();

    delete &as;
    delete &colas;

    deepErase( cache );
    deepErase( colcache );
}


void visSurvey::RandomTrackDisplay::setRandomTrack( visBase::RandomTrack* rt )
{
    if ( track )
    {
	track->knotmovement.remove( mCB(this,RandomTrackDisplay,knotMoved) );
	track->knotnrchange.remove( mCB(this,RandomTrackDisplay,knotNrChanged));
	track->rightclick.remove( mCB(this,RandomTrackDisplay,rightClicked) );
	track->unRef();
    }

    track = rt;
    track->ref();
    track->rightclick.notify( mCB(this,RandomTrackDisplay,rightClicked) );
    track->knotmovement.notify( mCB(this,RandomTrackDisplay,knotMoved) );
    track->knotnrchange.notify( mCB(this,RandomTrackDisplay,knotNrChanged) );
}


void visSurvey::RandomTrackDisplay::setSelSpec( const AttribSelSpec& as_ )
{
    as = as_;
    track->useTexture( false );
    setName( as.userRef() );
}


void visSurvey::RandomTrackDisplay::setColorSelSpec( const ColorAttribSel& as_ )
{ colas = as_; }


void visSurvey::RandomTrackDisplay::setDepthInterval( 
						const Interval<float>& intv )
{ 
    track->setDepthInterval( intv );
    moving.trigger();
}


Interval<float> visSurvey::RandomTrackDisplay::getDataTraceRange() const
{ return track->getDraggerDepthInterval(); }


int visSurvey::RandomTrackDisplay::nrKnots() const
{ return track->nrKnots(); }


void visSurvey::RandomTrackDisplay::addKnot( const BinID& bid_ )
{
    const BinID bid = snapPosition( bid_ );
    if ( checkPosition(bid) )
    {
	track->addKnot( Coord(bid.inl,bid.crl) );
	moving.trigger();
    }
}


void visSurvey::RandomTrackDisplay::insertKnot( int knotidx, const BinID& bid_ )
{
    const BinID bid = snapPosition(bid_);
    if ( checkPosition(bid) )
    {
	track->insertKnot( knotidx, Coord(bid.inl,bid.crl) ); 
	moving.trigger();
    }
}


BinID visSurvey::RandomTrackDisplay::getKnotPos( int knotidx ) const
{
    Coord crd = track->getKnotPos( knotidx );
    return BinID( (int)crd.x, (int)crd.y ); 
}


BinID visSurvey::RandomTrackDisplay::getManipKnotPos( int knotidx ) const
{
    Coord crd = track->getDraggerKnotPos( knotidx );
    return BinID( (int)crd.x, (int)crd.y );
}


void visSurvey::RandomTrackDisplay::getAllKnotPos( TypeSet<BinID>& bidset ) const
{
    const int nrknots = track->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
	bidset += getManipKnotPos( idx );
}


void visSurvey::RandomTrackDisplay::setKnotPos( int knotidx, const BinID& bid_ )
{
    const BinID bid = snapPosition(bid_);
    if ( checkPosition(bid) )
    {
	track->setKnotPos( knotidx, Coord(bid.inl,bid.crl) );
	moving.trigger();
    }
}


void visSurvey::RandomTrackDisplay::removeKnot( int knotidx )
{ track->removeKnot( knotidx ); }


void visSurvey::RandomTrackDisplay::removeAllKnots()
{
    const int nrknots = track->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
	track->removeKnot( idx );
}


#define mGetBinIDs( x, y ) \
    bool reverse = stop.x - start.x < 0; \
    int step = inlwise ? SI().inlWorkStep() : SI().crlWorkStep(); \
    if ( reverse ) step *= -1; \
    for ( int idi=0; idi<nrlines; idi++ ) \
    { \
	BinID bid; \
	int bidx = start.x + idi*step; \
	float val = linearInterpolate( (float)start.x, (float)start.y, \
				       (float)stop.x, (float)stop.y, \
				       (float)bidx ); \
	int bidy = (int)(val + .5); \
	BinID nextbid = inlwise ? BinID(bidx,bidy) : BinID(bidy,bidx); \
	SI().snap( nextbid ); \
	bids += nextbid ; \
	(*bset) += nextbid; \
    }


void visSurvey::RandomTrackDisplay::getDataTraceBids(
				    TypeSet<BinID>& bids ) const
{
    deepErase( const_cast<RandomTrackDisplay*>(this)->bidsset );
    TypeSet<BinID> bidset;
    getAllKnotPos( bidset );
    for ( int idx=1; idx<bidset.size(); idx++ )
    {
	TypeSet<BinID>* bset = new TypeSet<BinID>;
	const_cast<RandomTrackDisplay*>(this)->bidsset += bset;
	BinID start = bidset[idx-1];
	BinID stop = bidset[idx];
	const int nrinl = int(abs(stop.inl-start.inl) / SI().inlWorkStep() + 1);
	const int nrcrl = int(abs(stop.crl-start.crl) / SI().crlWorkStep() + 1);
	bool inlwise = nrinl > nrcrl;
	int nrlines = inlwise ? nrinl : nrcrl;
	if ( inlwise )
	{ mGetBinIDs(inl,crl); }
	else 
	{ mGetBinIDs(crl,inl); }
    }
}


void visSurvey::RandomTrackDisplay::setTraceData( bool colordata,
						  ObjectSet<SeisTrc>* trcset )
{
    if ( !trcset )
    {
	const int nrsections = bidsset.size();
	for ( int snr=0; snr<nrsections; snr++ )
	    track->setData(snr,0,0);
	return;
    }
    
    const int nrtrcs = trcset->size();
    if ( !nrtrcs ) return;

    if ( colordata )
    {
	Interval<float> cliprate( colas.cliprate0, colas.cliprate1 );
	track->setColorPars( colas.reverse, colas.useclip,
			     colas.useclip ? cliprate : colas.range );
    }
    
    setData( *trcset, colordata ? colas.datatype : 0 );
    if ( colordata )
    {
	deepErase( colcache );
	colcache = *trcset;
	return;
    }

    deepErase( cache );
    cache = *trcset;
    ismanip = false;
}


void visSurvey::RandomTrackDisplay::setData( const ObjectSet<SeisTrc>& trcset,
					     int datatype )
{
    const Interval<float> zrg = getDataTraceRange();
    const float step = trcset[0]->info().sampling.step;
    const int nrsamp = mNINT( zrg.width() / step ) + 1;

    const int nrsections = bidsset.size();
    for ( int snr=0; snr<nrsections; snr++ )
    {
	TypeSet<BinID> binidset = *(bidsset[snr]);
	const int nrbids = binidset.size();
	PtrMan<Array2DImpl<float> > arr = new Array2DImpl<float>(nrsamp,nrbids);
	for ( int bidnr=0; bidnr<nrbids; bidnr++ )
	{
	    BinID curbid = binidset[bidnr];
	    const SeisTrc* trc = getTrc( curbid, trcset );
	    if ( !trc ) continue;

	    float ctime = zrg.start;
	    for ( int ids=0; ids<nrsamp; ids++ )
	    {
		arr->set( ids, bidnr, trc->getValue(ctime,0) );
		ctime += step;
	    }
	}
	
	track->setData( snr, arr, datatype );
    }

    track->useTexture( true );
}


const SeisTrc* visSurvey::RandomTrackDisplay::getTrc( const BinID& bid, 
					const ObjectSet<SeisTrc>& trcset ) const
{
    const int nrtrcs = trcset.size();
    for ( int trcidx=0; trcidx<nrtrcs; trcidx++ )
    {
	if ( trcset[trcidx]->info().binid == bid )
	    return trcset[trcidx];
    }

    return 0;
}


float visSurvey::RandomTrackDisplay::getValue( const Coord3& pos ) const
{
    if ( !cache.size() ) return 0;

    BinID bid( (int)pos.x, (int)pos.y );
    const SeisTrc* trc = getTrc( bid, cache );
    if ( !trc ) return 0;

    int sampidx = trc->nearestSample( pos.z, 0 );
    return trc->get( sampidx, 0 );
}

bool visSurvey::RandomTrackDisplay::canAddKnot(int knotnr) const
{
    if ( knotnr<0 ) knotnr=0;
    if ( knotnr>nrKnots() ) knotnr=nrKnots();

    const BinID newpos = proposeNewPos(knotnr);
    return checkPosition(newpos);
}


void visSurvey::RandomTrackDisplay::addKnot(int knotnr)
{
    if ( knotnr<0 ) knotnr=0;
    if ( knotnr>nrKnots() ) knotnr=nrKnots();

    if ( !canAddKnot(knotnr) ) return;

    const BinID newpos = proposeNewPos(knotnr);
    if ( knotnr==nrKnots() )
	addKnot(newpos);
    else insertKnot(knotnr, newpos );
}
    

BinID visSurvey::RandomTrackDisplay::proposeNewPos(int knotnr ) const
{
    BinID res;
    if ( !knotnr )
	res = getKnotPos(0)-(getKnotPos(1)-getKnotPos(0));
    else if ( knotnr>=nrKnots() )
	res = getKnotPos(nrKnots()-1) +
	      (getKnotPos(nrKnots()-1)-getKnotPos(nrKnots()-2));
    else
    {
	res = getKnotPos(knotnr)+getKnotPos(knotnr-1);
	res.inl /= 2;
	res.crl /= 2;
    }

    res.inl = mMIN( SI().inlRange(true).stop, res.inl );
    res.inl = mMAX( SI().inlRange(true).start, res.inl );
    res.crl = mMIN( SI().crlRange(true).stop, res.crl );
    res.crl = mMAX( SI().crlRange(true).start, res.crl );

    SI().snap(res, BinID(0,0), true );

    return res;
}





bool visSurvey::RandomTrackDisplay::isManipulated() const
{
    return ismanip;
}

 
void visSurvey::RandomTrackDisplay::acceptManipulation()
{
    track->moveObjectToDraggerPos();
}


void visSurvey::RandomTrackDisplay::resetManipulation()
{
    track->moveDraggerToObjectPos();
    ismanip = false;
}


void visSurvey::RandomTrackDisplay::showManipulator( bool yn )
{ track->showDragger( yn ); }


bool visSurvey::RandomTrackDisplay::isManipulatorShown() const
{ return track->isDraggerShown(); }
 

void visSurvey::RandomTrackDisplay::turnOn( bool yn )
{ track->turnOn( yn ); }


bool visSurvey::RandomTrackDisplay::isOn() const
{ return track->isOn(); }


int visSurvey::RandomTrackDisplay::getColTabID() const
{ return track->getColorTab().id(); }


const TypeSet<float>* visSurvey::RandomTrackDisplay::getHistogram() const
{ return &track->getHistogram(); }


void visSurvey::RandomTrackDisplay::rightClicked( CallBacker* )
{ rightclick.trigger(); }


void visSurvey::RandomTrackDisplay::knotMoved( CallBacker* cb )
{
    ismanip = true;
    mCBCapsuleUnpack(int,sel,cb);
    
    selknotidx = sel;
    knotmoving.trigger();
    moving.trigger();
}


void visSurvey::RandomTrackDisplay::knotNrChanged( CallBacker* )
{
    ismanip = true;
    if ( !cache.size() )
	return;

    TypeSet<BinID> bids;
    getDataTraceBids( bids );
    setData( cache );
}


bool visSurvey::RandomTrackDisplay::checkPosition( const BinID& binid ) const
{
    const BinIDRange rg = SI().range();
    if ( binid.inl < rg.start.inl ) return false;
    if ( binid.inl > rg.stop.inl ) return false;
    if ( binid.crl < rg.start.crl ) return false;
    if ( binid.crl > rg.stop.crl ) return false;

    BinID snapped( binid );
    SI().snap(snapped, BinID(0,0), true );
    if ( snapped!=binid )
	return false;

    for ( int idx=0; idx<nrKnots(); idx++ )
	if ( getKnotPos(idx)==binid )
	    return false;

    return true;
}


BinID visSurvey::RandomTrackDisplay::snapPosition( const BinID& binid_ ) const
{
    BinID binid( binid_ );
    const BinIDRange rg = SI().range();
    if ( binid.inl < rg.start.inl ) binid.inl = rg.start.inl;
    if ( binid.inl > rg.stop.inl ) binid.inl = rg.stop.inl;
    if ( binid.crl < rg.start.crl ) binid.crl = rg.start.crl;
    if ( binid.crl > rg.stop.crl ) binid.crl = rg.stop.crl;

    SI().snap(binid, BinID(0,0), true );
    return binid;
}


void visSurvey::RandomTrackDisplay::setResolution( int res )
{
    track->setResolution( res );
    if ( cache.size() ) setData( cache );
    if ( colcache.size() ) setData( colcache, colas.datatype );
}


int visSurvey::RandomTrackDisplay::getResolution() const
{
    return track->getResolution();
}


BufferString visSurvey::RandomTrackDisplay::getResolutionName( int res ) const
{
    const char* name;
    if ( res == 1 ) 
	name = "Moderate";
    else if ( res == 2 ) 
	name = "High";
    else 
	name = "Default";

    return BufferString(name);
}


int visSurvey::RandomTrackDisplay::nrResolutions() const
{ return 3; }


int visSurvey::RandomTrackDisplay::getSectionIdx() const
{ return track->getSectionIdx(); }


BinID visSurvey::RandomTrackDisplay::getClickedPos() const
{ 
    Coord crd = track->getClickedPos();
    return BinID( (int)crd.x, (int)crd.y );
}


void visSurvey::RandomTrackDisplay::removeNearestKnot( int sectionidx, 
						       const BinID& pos_ )
{
    Coord pos( pos_.inl, pos_.crl );
    if ( pos.distance( track->getKnotPos(sectionidx) ) <
	 pos.distance( track->getKnotPos(sectionidx+1) ) )
	removeKnot( sectionidx );
    else
	removeKnot( sectionidx+1 );
}


float visSurvey::RandomTrackDisplay::calcDist( const Coord3& pos ) const
{
    const visBase::Transformation* utm2display= SPM().getUTM2DisplayTransform();
    Coord3 xytpos = utm2display->transformBack( pos );
    BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );
    if ( !getTrc(binid,cache) ) return mUndefValue;

    float zdiff = 0;
    const Interval<float>& intv = track->getDepthInterval();
    if ( xytpos.z < intv.start )
	zdiff = intv.start - xytpos.z;
    else if ( xytpos.z > intv.stop )
	zdiff = xytpos.z - intv.stop;

    return zdiff;
}


void visSurvey::RandomTrackDisplay::setMaterial( visBase::Material* nm)
{ track->setMaterial(nm); }


const visBase::Material* visSurvey::RandomTrackDisplay::getMaterial() const
{ return track->getMaterial(); }


visBase::Material* visSurvey::RandomTrackDisplay::getMaterial()
{ return track->getMaterial(); }


SoNode* visSurvey::RandomTrackDisplay::getInventorNode()
{ return track->getInventorNode(); }


void visSurvey::RandomTrackDisplay::fillPar( IOPar& par, TypeSet<int>& saveids )
									   const
{
    visBase::VisualObject::fillPar( par, saveids );

    int trackid = track->id();
    par.set( trackstr, trackid );

    const Interval<float> depthrg = track->getDepthInterval();
    par.set( depthintvstr, depthrg.start, depthrg.stop );

    int nrknots = nrKnots();
    par.set( nrknotsstr, nrknots );

    BufferString key;
    for ( int idx=0; idx<nrknots; idx++ )
    {
	key = knotprefix;
	key += idx;
	BinID bid = getKnotPos( idx );
	par.set( key, bid.inl, bid.crl );
    }

    if ( saveids.indexOf(trackid) == -1 ) saveids += trackid;

    as.fillPar(par);
    colas.fillPar(par);
}


int visSurvey::RandomTrackDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObject::usePar( par );
    if ( res != 1 ) return res;

    int trackid;
    if ( !par.get( trackstr, trackid ) ) return -1;
    visBase::DataObject* dataobj = visBase::DM().getObj( trackid );
    if ( !dataobj ) return 0;
    mDynamicCastGet(visBase::RandomTrack*,rt,dataobj);
    if ( !rt ) return -1;
    setRandomTrack( rt );

    Interval<float> intv(0,1);
    par.get( depthintvstr, intv.start, intv.stop );
    setDepthInterval( intv );

    int nrknots = 0;
    par.get( nrknotsstr, nrknots );

    BufferString key;
    for ( int idx=0; idx<nrknots; idx++ )
    {
	key = knotprefix;
	key += idx;
	BinID pos;
	par.get( key, pos.inl, pos.crl );
	if ( idx < 2 )
	    setKnotPos( idx, pos );
	else
	    addKnot( pos );
    }

    const StepInterval<double>& survinterval = SI().zRange(true);
    const StepInterval<float> inlrange( SI().range(true).start.inl,
	    				SI().range(true).stop.inl,
					SI().inlWorkStep() );
    const StepInterval<float> crlrange( SI().range(true).start.crl,
	    				SI().range(true).stop.crl,
	    				SI().crlWorkStep() );

    track->setXrange( StepInterval<float>( inlrange.start,
		    			   inlrange.stop,
					   inlrange.step ));

    track->setYrange( StepInterval<float>( crlrange.start,
	    				   crlrange.stop,
					   crlrange.step ));

    track->setZrange( StepInterval<float>( survinterval.start,
					   survinterval.stop,
					   survinterval.step ));

    if ( !as.usePar(par) ) return -1;
    colas.usePar( par );

    showManipulator(true);
    showManipulator(false);
    return 1;
}
