/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          January 2003
 RCS:           $Id: visrandomtrackdisplay.cc,v 1.42 2004-09-17 15:13:39 nanne Exp $
 ________________________________________________________________________

-*/


#include "visrandomtrackdisplay.h"

#include "arrayndimpl.h"
#include "attribsel.h"
#include "binidselimpl.h"
#include "iopar.h"
#include "seisbuf.h"
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

mCreateFactoryEntry( visSurvey::RandomTrackDisplay );

namespace visSurvey
{

const char* RandomTrackDisplay::trackstr = "Random track";
const char* RandomTrackDisplay::nrknotsstr = "Nr. Knots";
const char* RandomTrackDisplay::knotprefix = "Knot ";
const char* RandomTrackDisplay::depthintvstr = "Depth Interval";



RandomTrackDisplay::RandomTrackDisplay()
    : VisualObject(true)
    , track(0)
    , texturematerial(visBase::Material::create())
    , as(*new AttribSelSpec)
    , colas(*new ColorAttribSel)
    , knotmoving(this)
    , moving(this)
    , selknotidx(-1)
    , ismanip( true )
    , cache(*new SeisTrcBuf(true))
    , colcache(*new SeisTrcBuf(true))
{
    setRandomTrack( visBase::RandomTrack::create() );

    texturematerial->ref();
    texturematerial->setAmbience( 0.8 );
    texturematerial->setDiffIntensity( 0.8 );
    track->setMaterial( texturematerial );

    const StepInterval<float>& survinterval = SI().zRange(true);
    const StepInterval<float> inlrange( SI().sampling(true).hrg.start.inl,
	    				SI().sampling(true).hrg.stop.inl,
					SI().inlStep(true) );
    const StepInterval<float> crlrange( SI().sampling(true).hrg.start.crl,
	    				SI().sampling(true).hrg.stop.crl,
	    				SI().crlStep(true) );

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


RandomTrackDisplay::~RandomTrackDisplay()
{
    track->knotmovement.remove( mCB(this,RandomTrackDisplay,knotMoved) );
    track->knotnrchange.remove( mCB(this,RandomTrackDisplay,knotNrChanged) );
    track->unRef();
    texturematerial->unRef();

    delete &as;
    delete &colas;
    delete &cache;
    delete &colcache;
}


void RandomTrackDisplay::setRandomTrack( visBase::RandomTrack* rt )
{
    if ( track )
    {
	track->knotmovement.remove( mCB(this,RandomTrackDisplay,knotMoved) );
	track->knotnrchange.remove( mCB(this,RandomTrackDisplay,knotNrChanged));
	track->unRef();
    }

    track = rt;
    track->ref();
    track->setSelectable(false);
    track->knotmovement.notify( mCB(this,RandomTrackDisplay,knotMoved) );
    track->knotnrchange.notify( mCB(this,RandomTrackDisplay,knotNrChanged) );
}


void RandomTrackDisplay::setSelSpec( const AttribSelSpec& as_ )
{
    as = as_;
    track->useTexture( false );
    setName( as.userRef() );
}


void RandomTrackDisplay::setColorSelSpec( const ColorAttribSel& as_ )
{ colas = as_; }


void RandomTrackDisplay::setDepthInterval( const Interval<float>& intv )
{ 
    track->setDepthInterval( intv );
    moving.trigger();
}


Interval<float> RandomTrackDisplay::getDataTraceRange() const
{ return track->getDraggerDepthInterval(); }


int RandomTrackDisplay::nrKnots() const
{ return track->nrKnots(); }


void RandomTrackDisplay::addKnot( const BinID& bid_ )
{
    const BinID bid = snapPosition( bid_ );
    if ( checkPosition(bid) )
    {
	track->addKnot( Coord(bid.inl,bid.crl) );
	moving.trigger();
    }
}


void RandomTrackDisplay::insertKnot( int knotidx, const BinID& bid_ )
{
    const BinID bid = snapPosition(bid_);
    if ( checkPosition(bid) )
    {
	track->insertKnot( knotidx, Coord(bid.inl,bid.crl) ); 
	moving.trigger();
    }
}


BinID RandomTrackDisplay::getKnotPos( int knotidx ) const
{
    Coord crd = track->getKnotPos( knotidx );
    return BinID( (int)crd.x, (int)crd.y ); 
}


BinID RandomTrackDisplay::getManipKnotPos( int knotidx ) const
{
    Coord crd = track->getDraggerKnotPos( knotidx );
    return BinID( (int)crd.x, (int)crd.y );
}


void RandomTrackDisplay::getAllKnotPos( TypeSet<BinID>& bidset ) const
{
    const int nrknots = track->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
	bidset += getManipKnotPos( idx );
}


void RandomTrackDisplay::setKnotPos( int knotidx, const BinID& bid_ )
{
    const BinID bid = snapPosition(bid_);
    if ( checkPosition(bid) )
    {
	track->setKnotPos( knotidx, Coord(bid.inl,bid.crl) );
	moving.trigger();
    }
}


void RandomTrackDisplay::setManipKnotPos( int knotidx, const BinID& bid_ )
{
    const BinID bid = snapPosition(bid_);
    track->setDraggerKnotPos( knotidx, Coord(bid.inl,bid.crl) );
}


void RandomTrackDisplay::removeKnot( int knotidx )
{ track->removeKnot( knotidx ); }


void RandomTrackDisplay::removeAllKnots()
{
    const int nrknots = track->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
	track->removeKnot( idx );
}


#define mGetBinIDs( x, y ) \
    bool reverse = stop.x - start.x < 0; \
    int step = inlwise ? SI().inlStep(true) : SI().crlStep(true); \
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


void RandomTrackDisplay::getDataTraceBids( TypeSet<BinID>& bids ) const
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
	const int nrinl = int(abs(stop.inl-start.inl) / SI().inlStep(true) + 1);
	const int nrcrl = int(abs(stop.crl-start.crl) / SI().crlStep(true) + 1);
	bool inlwise = nrinl > nrcrl;
	int nrlines = inlwise ? nrinl : nrcrl;
	if ( inlwise )
	{ mGetBinIDs(inl,crl); }
	else 
	{ mGetBinIDs(crl,inl); }
    }
}


void RandomTrackDisplay::setTraceData( bool colordata, SeisTrcBuf& trcbuf )
{
    const int nrtrcs = trcbuf.size();
    if ( !nrtrcs )
    {
	const int nrsections = bidsset.size();
	for ( int snr=0; snr<nrsections; snr++ )
	    track->setData(snr,0,0);
	return;
    }

    if ( colordata )
    {
	Interval<float> cliprate( colas.cliprate0, colas.cliprate1 );
	track->setColorPars( colas.reverse, colas.useclip,
			     colas.useclip ? cliprate : colas.range );
    }
    
    setData( trcbuf, colordata ? colas.datatype : 0 );
    SeisTrcBuf& cach = colordata ? colcache : cache;
    cach.deepErase();
    cach.stealTracesFrom( trcbuf );
    if ( !colordata )
	ismanip = false;
}


void RandomTrackDisplay::setData( const SeisTrcBuf& trcbuf, int datatype )
{
    const Interval<float> zrg = getDataTraceRange();
    const float step = trcbuf.get(0)->info().sampling.step;
    const int nrsamp = mNINT( zrg.width() / step ) + 1;

    const int nrsections = bidsset.size();
    for ( int snr=0; snr<nrsections; snr++ )
    {
	TypeSet<BinID>& binidset = *(bidsset[snr]);
	const int nrbids = binidset.size();
	float val;
	PtrMan<Array2DImpl<float> > arr = new Array2DImpl<float>(nrsamp,nrbids);
	for ( int bidnr=0; bidnr<nrbids; bidnr++ )
	{
	    BinID curbid = binidset[bidnr];
	    int trcidx = trcbuf.find( curbid );
	    if ( trcidx < 0 )
	    {
		for ( int ids=0; ids<nrsamp; ids++ )
		    arr->set( ids, bidnr, mUndefValue );
		continue;
	    }

	    const SeisTrc* trc = trcbuf.get( trcidx );
	    float ctime = zrg.start;
	    for ( int ids=0; ids<nrsamp; ids++ )
	    {
		val = trc && trc->dataPresent(ctime,0) ? trc->getValue(ctime,0)
						       : mUndefValue;
		arr->set( ids, bidnr, val );
		ctime += step;
	    }
	}
	
	track->setData( snr, arr, datatype );
    }

    track->useTexture( true );
}


#define mFindTrc(inladd,crladd) \
    if ( trcidx < 0 ) \
    { \
	bid.inl = reqbid.inl + step.inl * (inladd); \
	bid.crl = reqbid.crl + step.crl * (crladd); \
	trcidx = cache.find( bid ); \
    }


float RandomTrackDisplay::getValue( const Coord3& pos ) const
{
    if ( !cache.size() ) return mUndefValue;

    BinID reqbid( SI().transform(pos) );
    int trcidx = cache.find( reqbid );
    if ( trcidx < 0 )
    {
	const BinID step( SI().inlStep(true), SI().crlStep(true) );
	BinID bid;
	mFindTrc(1,0) mFindTrc(-1,0) mFindTrc(0,1) mFindTrc(0,-1)
	if ( trcidx < 0 )
	{
	    mFindTrc(1,1) mFindTrc(-1,1) mFindTrc(1,-1) mFindTrc(-1,-1)
	}

	if ( trcidx < 0 )
	    return mUndefValue;
    }

    const SeisTrc& trc = *cache.get( trcidx );
    const int sampidx = trc.nearestSample( pos.z, 0 );
    return sampidx < 0 || sampidx >= trc.size(0) ? mUndefValue
						 : trc.get( sampidx, 0 );
}


bool RandomTrackDisplay::canAddKnot( int knotnr ) const
{
    if ( knotnr<0 ) knotnr=0;
    if ( knotnr>nrKnots() ) knotnr=nrKnots();

    const BinID newpos = proposeNewPos(knotnr);
    return checkPosition(newpos);
}


void RandomTrackDisplay::addKnot( int knotnr )
{
    if ( knotnr<0 ) knotnr=0;
    if ( knotnr>nrKnots() ) knotnr=nrKnots();

    if ( !canAddKnot(knotnr) ) return;

    const BinID newpos = proposeNewPos(knotnr);
    if ( knotnr==nrKnots() )
	addKnot(newpos);
    else insertKnot(knotnr, newpos );
}
    

BinID RandomTrackDisplay::proposeNewPos(int knotnr ) const
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


bool RandomTrackDisplay::isManipulated() const
{
    return ismanip;
}

 
void RandomTrackDisplay::acceptManipulation()
{
    track->moveObjectToDraggerPos();
    moving.trigger();
}


void RandomTrackDisplay::resetManipulation()
{
    track->moveDraggerToObjectPos();
    ismanip = false;
}


void RandomTrackDisplay::showManipulator( bool yn )
{ track->showDragger( yn ); }


bool RandomTrackDisplay::isManipulatorShown() const
{ return track->isDraggerShown(); }


BufferString RandomTrackDisplay::getManipulationString() const
{
    BufferString str;
    int knotidx = getSelKnotIdx();
    if ( knotidx >= 0 )
    {
	BinID binid  = getManipKnotPos( knotidx );
	str = "Node "; str += knotidx;
	str += " Inl/Crl: ";
	str += binid.inl; str += "/"; str += binid.crl;
    }
    return str;
}
 

void RandomTrackDisplay::turnOn( bool yn )
{ track->turnOn( yn ); }


bool RandomTrackDisplay::isOn() const
{ return track->isOn(); }


int RandomTrackDisplay::getColTabID() const
{ return track->getColorTab().id(); }


const TypeSet<float>* RandomTrackDisplay::getHistogram() const
{ return &track->getHistogram(); }


void RandomTrackDisplay::knotMoved( CallBacker* cb )
{
    ismanip = true;
    mCBCapsuleUnpack(int,sel,cb);
    
    selknotidx = sel;
    knotmoving.trigger();
}


void RandomTrackDisplay::knotNrChanged( CallBacker* )
{
    ismanip = true;
    if ( !cache.size() )
	return;

    TypeSet<BinID> bids;
    getDataTraceBids( bids );
    setData( cache );
}


bool RandomTrackDisplay::checkPosition( const BinID& binid ) const
{
    const HorSampling& hs = SI().sampling(true).hrg;
    if ( !hs.includes(binid) )
	return false;

    BinID snapped( binid );
    SI().snap(snapped, BinID(0,0), true );
    if ( snapped != binid )
	return false;

    for ( int idx=0; idx<nrKnots(); idx++ )
	if ( getKnotPos(idx) == binid )
	    return false;

    return true;
}


BinID RandomTrackDisplay::snapPosition( const BinID& binid_ ) const
{
    BinID binid( binid_ );
    const HorSampling& hs = SI().sampling(true).hrg;
    if ( binid.inl < hs.start.inl ) binid.inl = hs.start.inl;
    if ( binid.inl > hs.stop.inl ) binid.inl = hs.stop.inl;
    if ( binid.crl < hs.start.crl ) binid.crl = hs.start.crl;
    if ( binid.crl > hs.stop.crl ) binid.crl = hs.stop.crl;

    SI().snap( binid, BinID(0,0), true );
    return binid;
}


void RandomTrackDisplay::setResolution( int res )
{
    track->setResolution( res );
    if ( cache.size() ) setData( cache );
    if ( colcache.size() ) setData( colcache, colas.datatype );
}


int RandomTrackDisplay::getResolution() const
{
    return track->getResolution();
}


int RandomTrackDisplay::nrResolutions() const
{ return 3; }


int RandomTrackDisplay::getSectionIdx() const
{ return track->getSectionIdx(); }


void RandomTrackDisplay::removeNearestKnot( int sectionidx, 
						       const BinID& pos_ )
{
    Coord pos( pos_.inl, pos_.crl );
    if ( pos.distance( track->getKnotPos(sectionidx) ) <
	 pos.distance( track->getKnotPos(sectionidx+1) ) )
	removeKnot( sectionidx );
    else
	removeKnot( sectionidx+1 );
}


float RandomTrackDisplay::calcDist( const Coord3& pos ) const
{
    const visBase::Transformation* utm2display= SPM().getUTM2DisplayTransform();
    Coord3 xytpos = utm2display->transformBack( pos );
    BinID binid = SI().transform( Coord(xytpos.x,xytpos.y) );
    if ( cache.find(binid) < 0 ) return mUndefValue;

    float zdiff = 0;
    const Interval<float>& intv = track->getDepthInterval();
    if ( xytpos.z < intv.start )
	zdiff = intv.start - xytpos.z;
    else if ( xytpos.z > intv.stop )
	zdiff = xytpos.z - intv.stop;

    return zdiff;
}


void RandomTrackDisplay::setMaterial( visBase::Material* nm)
{ track->setMaterial(nm); }


const visBase::Material* RandomTrackDisplay::getMaterial() const
{ return track->getMaterial(); }


visBase::Material* RandomTrackDisplay::getMaterial()
{ return track->getMaterial(); }


SoNode* RandomTrackDisplay::getInventorNode()
{ return track->getInventorNode(); }


void RandomTrackDisplay::fillPar( IOPar& par, TypeSet<int>& saveids )
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


int RandomTrackDisplay::usePar( const IOPar& par )
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

    const StepInterval<float>& survinterval = SI().zRange(true);
    const StepInterval<float> inlrange( SI().sampling(true).hrg.start.inl,
	    				SI().sampling(true).hrg.stop.inl,
					SI().inlStep(true) );
    const StepInterval<float> crlrange( SI().sampling(true).hrg.start.crl,
	    				SI().sampling(true).hrg.stop.crl,
	    				SI().crlStep(true) );

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

}; // namespace visSurvey
