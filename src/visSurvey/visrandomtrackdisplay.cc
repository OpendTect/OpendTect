/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          January 2003
 RCS:           $Id: visrandomtrackdisplay.cc,v 1.27 2003-11-07 12:22:03 bert Exp $
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

    SI().snap( start );
    SI().snap( stop );

    track->setKnotPos( 0, Coord( start.inl, start.crl ) );
    track->setKnotPos( 1, Coord( stop.inl, stop.crl ) );

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
    showDragger(true);
    showDragger(false);
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


AttribSelSpec& visSurvey::RandomTrackDisplay::getAttribSelSpec()
{ return as; }


const AttribSelSpec& visSurvey::RandomTrackDisplay::getAttribSelSpec() const
{ return as; }


void visSurvey::RandomTrackDisplay::setAttribSelSpec( const AttribSelSpec& as_ )
{
    as = as_;
    colas.datatype = 0;
    track->useTexture( false );
    setName( as.userRef() );
}


ColorAttribSel& visSurvey::RandomTrackDisplay::getColorSelSpec()
{ return colas; }


const ColorAttribSel& visSurvey::RandomTrackDisplay::getColorSelSpec() const
{ return colas; }


void visSurvey::RandomTrackDisplay::setColorSelSpec( const ColorAttribSel& as_ )
{ colas = as_; }


void visSurvey::RandomTrackDisplay::setDepthInterval( 
						const Interval<float>& intv )
{ 
    track->setDepthInterval( intv );
    moving.trigger();
}


const Interval<float> visSurvey::RandomTrackDisplay::getDepthInterval() const
{ return track->getDepthInterval(); }


const Interval<float>
visSurvey::RandomTrackDisplay::getManipDepthInterval() const
{
    return track->getDraggerDepthInterval();
}


int visSurvey::RandomTrackDisplay::nrKnots() const
{ return track->nrKnots(); }


void visSurvey::RandomTrackDisplay::addKnot( const BinID& bid_ )
{
    BinID bid( bid_ );
    checkPosition( bid );
    track->addKnot( Coord(bid.inl,bid.crl) );
    moving.trigger();
}


void visSurvey::RandomTrackDisplay::addKnots( TypeSet<BinID> bidset )
{
    for ( int idx=0; idx<bidset.size(); idx++ )
	addKnot( bidset[idx] );
    moving.trigger();
}


void visSurvey::RandomTrackDisplay::insertKnot( int knotidx, const BinID& bid_ )
{
    BinID bid( bid_ );
    checkPosition( bid );
    track->insertKnot( knotidx, Coord(bid.inl,bid.crl) ); 
    moving.trigger();
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


void visSurvey::RandomTrackDisplay::getAllKnotPos( TypeSet<BinID>& bidset )
{
    const int nrknots = track->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
	bidset += getManipKnotPos( idx );
}


void visSurvey::RandomTrackDisplay::setKnotPos( int knotidx, const BinID& bid_ )
{
    BinID bid( bid_ );
    checkPosition( bid );
    track->setKnotPos( knotidx, Coord(bid.inl,bid.crl) );
    moving.trigger();
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


void visSurvey::RandomTrackDisplay::getDataPositions( TypeSet<BinID>& bids )
{
    deepErase( bidsset );
    TypeSet<BinID> bidset;
    getAllKnotPos( bidset );
    for ( int idx=1; idx<bidset.size(); idx++ )
    {
	TypeSet<BinID>* bset = new TypeSet<BinID>;
	bidsset += bset;
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


bool visSurvey::RandomTrackDisplay::putNewData( ObjectSet<SeisTrc>* trcset,
					        bool colordata )
{
    if ( !trcset )
    {
	const int nrsections = bidsset.size();
	for ( int snr=0; snr<nrsections; snr++ )
	    track->setData(snr,0,0);
	return true;
    }
    
    const int nrtrcs = trcset->size();
    if ( !nrtrcs ) return false;

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
	return true;
    }

    deepErase( cache );
    cache = *trcset;
    ismanip = false;
    return true;
}


void visSurvey::RandomTrackDisplay::setData( const ObjectSet<SeisTrc>& trcset,
					     int datatype )
{
    const Interval<float> zrg = getManipDepthInterval();
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


bool visSurvey::RandomTrackDisplay::isManipulated() const
{
    return ismanip;
}

 
void visSurvey::RandomTrackDisplay::acceptManip()
{
    track->moveObjectToDraggerPos();
}


void visSurvey::RandomTrackDisplay::showDragger( bool yn )
{ track->showDragger( yn ); }


bool visSurvey::RandomTrackDisplay::isDraggerShown() const
{ return track->isDraggerShown(); }
 

void visSurvey::RandomTrackDisplay::turnOn( bool yn )
{ track->turnOn( yn ); }


bool visSurvey::RandomTrackDisplay::isOn() const
{ return track->isOn(); }


void visSurvey::RandomTrackDisplay::setColorTab(visBase::VisColorTab& ctab)
{ track->setColorTab(ctab); }


visBase::VisColorTab& visSurvey::RandomTrackDisplay::getColorTab()
{ return track->getColorTab(); }


const visBase::VisColorTab& visSurvey::RandomTrackDisplay::getColorTab() const
{ return track->getColorTab(); }


const TypeSet<float>& visSurvey::RandomTrackDisplay::getHistogram() const
{ return track->getHistogram(); }


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
    getDataPositions( bids );
    setData( cache );
}


void visSurvey::RandomTrackDisplay::checkPosition( BinID& binid )
{
    const BinIDRange rg = SI().range();
    if ( binid.inl < rg.start.inl ) binid.inl = rg.start.inl;
    if ( binid.inl > rg.stop.inl ) binid.inl = rg.stop.inl;
    if ( binid.crl < rg.start.crl ) binid.crl = rg.start.crl;
    if ( binid.crl > rg.stop.crl ) binid.crl = rg.stop.crl;
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


const char* visSurvey::RandomTrackDisplay::getResName( int res ) const
{
    if ( res == 1 ) 
	return "Moderate";
    else if ( res == 2 ) 
	return "High";
    else 
	return "Default";
}


int visSurvey::RandomTrackDisplay::getNrResolutions() const
{
    return 3;
}


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


SoNode* visSurvey::RandomTrackDisplay::getData()
{ return track->getData(); }


void visSurvey::RandomTrackDisplay::fillPar( IOPar& par, TypeSet<int>& saveids )
									   const
{
    visBase::VisualObject::fillPar( par, saveids );

    int trackid = track->id();
    par.set( trackstr, trackid );

    const Interval<float> depthrg = getDepthInterval();
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

    showDragger(true);
    showDragger(false);
    return 1;
}
