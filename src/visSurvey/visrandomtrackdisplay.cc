/*+
 ________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          January 2003
 RCS:           $Id: visrandomtrackdisplay.cc,v 1.9 2003-02-19 16:10:39 nanne Exp $
 ________________________________________________________________________

-*/


#include "visrandomtrackdisplay.h"

#include "arrayndimpl.h"
#include "attribsel.h"
#include "binidselimpl.h"
#include "iopar.h"
#include "seistrc.h"
#include "simpnumer.h"
#include "survinfo.h"
#include "visdataman.h"
#include "vismaterial.h"
#include "visrandomtrack.h"

#include <math.h>


mCreateFactoryEntry( visSurvey::RandomTrackDisplay );

visSurvey::RandomTrackDisplay::RandomTrackDisplay()
    : VisualObject(true)
    , track(visBase::RandomTrack::create())
    , texturematerial(visBase::Material::create())
    , as(*new AttribSelSpec)
    , knotmoving(this)
    , selknotidx(-1)
{
    track->ref();

    texturematerial->ref();
    texturematerial->setAmbience( 0.8 );
    texturematerial->setDiffIntensity( 0.8 );
    track->setMaterial( texturematerial );

    track->knotmovement.notify( mCB(this,RandomTrackDisplay,knotMoved) );

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
    track->unRef();
    texturematerial->unRef();
}


AttribSelSpec& visSurvey::RandomTrackDisplay::getAttribSelSpec()
{ return as; }


const AttribSelSpec& visSurvey::RandomTrackDisplay::getAttribSelSpec() const
{ return as; }


void visSurvey::RandomTrackDisplay::setAttribSelSpec( const AttribSelSpec& as_ )
{ as = as_; }


void visSurvey::RandomTrackDisplay::setDepthInterval( 
						const Interval<float>& intv )
{ track->setDepthInterval( intv ); }


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
}


void visSurvey::RandomTrackDisplay::addKnots( TypeSet<BinID> bidset )
{
    for ( int idx=0; idx<bidset.size(); idx++ )
	addKnot( bidset[idx] );
}


void visSurvey::RandomTrackDisplay::insertKnot( int knotidx, const BinID& bid_ )
{
    BinID bid( bid_ );
    checkPosition( bid );
    track->insertKnot( knotidx, Coord(bid.inl,bid.crl) ); 
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


void visSurvey::RandomTrackDisplay::setKnotPos( int knotidx, const BinID& bid )
{ track->setKnotPos( knotidx, Coord(bid.inl,bid.crl) ); }


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


bool visSurvey::RandomTrackDisplay::putNewData(const ObjectSet<SeisTrc>& trcset)
{
    const int nrtrcs = trcset.size();
    if ( !nrtrcs ) return false;
    
    const Interval<float> zrg = getManipDepthInterval();
    const float step = trcset[0]->info().sampling.step;
    const int nrsamp = mNINT( zrg.width() / step ) + 1;

    const int nrsections = bidsset.size();
    for ( int snr=0; snr<nrsections; snr++ )
    {
	TypeSet<BinID> binidset = *(bidsset[snr]);
	const int nrbids = binidset.size();
	Array2DImpl<float> arr( nrsamp, nrbids );
	for ( int bidnr=0; bidnr<nrbids; bidnr++ )
	{
	    BinID curbid = binidset[bidnr];
	    const SeisTrc* trc = getTrc( curbid, trcset );
	    if ( !trc ) continue;

	    float ctime = zrg.start;
	    for ( int ids=0; ids<nrsamp; ids++ )
	    {
		arr.set( ids, bidnr, trc->getValue(ctime,0) );
		ctime += step;
	    }
	}
	
	track->setData( snr, arr );
    }

    return true;
}


const SeisTrc* visSurvey::RandomTrackDisplay::getTrc( const BinID& bid, 
					const ObjectSet<SeisTrc>& trcset )
{
    const int nrtrcs = trcset.size();
    for ( int trcidx=0; trcidx<nrtrcs; trcidx++ )
    {
	if ( trcset[trcidx]->info().binid == bid )
	    return trcset[trcidx];
    }

    return 0;
}

 
void visSurvey::RandomTrackDisplay::acceptManip()
{
    const int nrknots = track->nrKnots();
    setDepthInterval( getManipDepthInterval() );
    for ( int idx=0; idx<nrknots; idx++ )
	setKnotPos( idx, getManipKnotPos( idx ) );
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


void visSurvey::RandomTrackDisplay::knotMoved( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    
    selknotidx = sel;
    knotmoving.trigger();
}


void visSurvey::RandomTrackDisplay::checkPosition( BinID& binid )
{
    const BinIDRange rg = SI().range();
    if ( binid.inl < rg.start.inl ) binid.inl = rg.start.inl;
    if ( binid.inl > rg.stop.inl ) binid.inl = rg.stop.inl;
    if ( binid.crl < rg.start.crl ) binid.crl = rg.start.crl;
    if ( binid.crl > rg.stop.crl ) binid.crl = rg.stop.crl;
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
}


int visSurvey::RandomTrackDisplay::usePar( const IOPar& par )
{
    int res =  visBase::VisualObject::usePar( par );
    if ( res!=1 ) return res;

    return 1;
}
