/*+
 ________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          January 2003
 RCS:           $Id: visrandomtrackdisplay.cc,v 1.5 2003-01-27 13:15:51 kristofer Exp $
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


mCreateFactoryEntry( visSurvey::RandomTrackDisplay );

visSurvey::RandomTrackDisplay::RandomTrackDisplay()
    : VisualObject(true)
    , track(visBase::RandomTrack::create())
    , as(*new AttribSelSpec)
    , selected_(false)
    , manipulated(false)
    , succeeded_(false)
{
    track->ref();
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
}


visSurvey::RandomTrackDisplay::~RandomTrackDisplay()
{
    track->unRef();
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


void visSurvey::RandomTrackDisplay::addKnot( const Coord& crd )
{ track->addKnot( crd ); }


void visSurvey::RandomTrackDisplay::addKnots( TypeSet<Coord> crdset )
{
    for ( int idx=0; idx<crdset.size(); idx++ )
    {
	track->addKnot( crdset[idx] );
    }
}


void visSurvey::RandomTrackDisplay::insertKnot( int knotidx, const Coord& crd )
{ track->insertKnot( knotidx, crd ); }


Coord visSurvey::RandomTrackDisplay::getKnotPos( int knotidx ) const
{ return track->getKnotPos( knotidx ); }


Coord visSurvey::RandomTrackDisplay::getManipKnotPos( int knotidx ) const
{ return track->getDraggerKnotPos( knotidx ); }


void visSurvey::RandomTrackDisplay::getAllKnotPos( TypeSet<Coord>& crdset )
{
    const int nrknots = track->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
	crdset += track->getKnotPos( idx );
}


void visSurvey::RandomTrackDisplay::setKnotPos( int knotidx, const Coord& crd )
{ track->setKnotPos( knotidx, crd ); }


void visSurvey::RandomTrackDisplay::removeKnot( int knotidx )
{ track->removeKnot( knotidx ); }


void visSurvey::RandomTrackDisplay::removeAllKnots()
{
    const int nrknots = track->nrKnots();
    for ( int idx=0; idx<nrknots; idx++ )
	track->removeKnot( idx );
}


#define mGetBinIDs( x, y ) \
    nrlines = stop.x - start.x; \
    if ( lastsection ) nrlines++; \
    for ( int idi=0; idi<nrlines; idi++ ) \
    { \
	BinID bid; \
	int bidx = start.x + idi; \
	float val = linearInterpolate( (float)start.x, (float)start.y, \
				       (float)stop.x, (float)stop.y, \
				       (float)bidx ); \
	int bidy = (int)(val + .5); \
	bids += inlwise ? BinID(bidx,bidy) : BinID(bidy,bidx); \
    }


void visSurvey::RandomTrackDisplay::getDataPositions( TypeSet<BinID>& bids )
{
    bool lastsection = false;
    trcspersection.erase();
    TypeSet<Coord> crdset;
    getAllKnotPos( crdset );
    for ( int idx=1; idx<crdset.size(); idx++ )
    {
	lastsection = idx == crdset.size()-1;
	BinID start = SI().transform( crdset[idx-1] );
	BinID stop = SI().transform( crdset[idx] );
	const int nrinl = stop.inl - start.inl + 1;
	const int nrcrl = stop.crl - start.crl + 1;
	bool inlwise = nrinl > nrcrl;
	int nrlines;
	if ( inlwise )
	{ mGetBinIDs(inl,crl); }
	else 
	{ mGetBinIDs(crl,inl); }

	trcspersection += nrlines + 1;
    }
}


bool visSurvey::RandomTrackDisplay::putNewData( ObjectSet<SeisTrc>& trcset )
{
    const int nrtrcs = trcset.size();
    if ( !nrtrcs ) return false;
    const int nrsamp = trcset[0]->size(0);

    int firsttrc = 0;
    int lasttrc = 1;
    const int nrsections = track->nrKnots() - 1;
    for ( int snr=0; snr<nrsections; snr++ )
    {
	int trcidx = 0;
	firsttrc += lasttrc - 1;
	const int nrinsection = trcspersection[snr];
	lasttrc += nrinsection - 1;
	Array2DImpl<float> arr( nrinsection, nrsamp );
	for ( int idx=firsttrc; idx<lasttrc; idx++ )
	{
	    if ( idx > nrtrcs ) return false;
	    SeisTrc* trc = trcset[idx];
	    if ( !trc ) continue;
	    
	    for ( int ids=0; ids<nrsamp; ids++ )
		arr.set( trcidx++, ids, trc->getValue(ids,0) );
	}

	track->setData( snr, arr );
    }

    return true;
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


void visSurvey::RandomTrackDisplay::setMaterial( visBase::Material* nm)
{ track->setMaterial(nm); }


const visBase::Material* visSurvey::RandomTrackDisplay::getMaterial() const
{ return track->getMaterial(); }


visBase::Material* visSurvey::RandomTrackDisplay::getMaterial()
{ return track->getMaterial(); }


void visSurvey::RandomTrackDisplay::select()
{
    if ( selected_ ) return;
    selected_ = true;
}


void visSurvey::RandomTrackDisplay::deSelect()
{
    if ( !selected_ ) return;
    selected_ = false;

/*
    if ( manipulated )
	updateAtNewPos();
*/
}


bool visSurvey::RandomTrackDisplay::updateAtNewPos()
{
    succeeded_ = false;
//  moved.trigger();
    manipulated = false;
    return succeeded_;
}


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
