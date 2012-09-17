/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: poly2horvol.cc,v 1.3 2010/06/18 12:23:27 cvskris Exp $";

#include "poly2horvol.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "pickset.h"
#include "survinfo.h"
#include "polygon.h"
#include "gridder2d.h"

#include <math.h>


Poly2HorVol::~Poly2HorVol()
{
    if ( hor_ )
	hor_->unRef();
}


void Poly2HorVol::setHorizon( EM::Horizon3D* hor )
{
    if ( hor_ )
	{ hor_->unRef(); hor_ = 0; }
    hor_ = hor;
    if ( hor_ )
	hor_->ref();
}


bool Poly2HorVol::setHorizon( const MultiID& mid, TaskRunner* tr )
{
    if ( hor_ )
	{ hor_->unRef(); hor_ = 0; }

    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( mid, tr );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)

    setHorizon( hor );
    return hor_;
}


#define mPolyLoc(b) \
	Geom::Point2D<float>( (float)b.inl, (float)(b.crl) )

float Poly2HorVol::getM3( float vel, bool upw, bool useneg )
{
    if ( !hor_ || ! ps_ )
	return mUdf(float);

    ODPolygon<float> poly;
    HorSampling hs;
    const Pick::Location& pl0( (*ps_)[0] );
    TypeSet<Coord> pts; TypeSet<float> zvals;
    for ( int idx=0; idx<ps_->size(); idx++ )
    {
	const Pick::Location& pl( (*ps_)[idx] );
	pts += pl.pos; zvals += pl.pos.z;
	const BinID bid( SI().transform(pl.pos) );
	poly.add( mPolyLoc(bid) );
	if ( idx )
	    hs.include( bid );
	else
	    hs.start = hs.stop = bid;
    }

    TriangulatedGridder2D grdr;
    grdr.setPoints( pts ); grdr.setValues( zvals, false );
    float avgz = 0;
    for ( int idx=0; idx<zvals.size(); idx++ )
	avgz += zvals[idx];
    avgz /= zvals.size();

    const int nrsect = hor_->nrSections();
    HorSamplingIterator iter( hs );
    BinID bid; float totth = 0;
    while ( iter.next(bid) )
    {
	if ( !poly.isInside(mPolyLoc(bid),true,1e-6) )
	    continue;

	const EM::SubID subid = bid.toInt64();
	const Coord coord( SI().transform(bid) );

	for ( int isect=0; isect<nrsect; isect++ )
	{
	    const EM::SectionID sid = hor_->sectionID( isect );
	    float horz = hor_->getPos( sid, subid ).z;
	    if ( mIsUdf(horz) && bid.inl!=hs.stop.inl && bid.crl!=hs.stop.crl )
 	    { //The very last edges should exclude.
		horz = hor_->geometry().sectionGeometry(sid)->computePosition(
       			Coord(bid.inl,bid.crl) ).z;
 	    }
		    
	    if ( mIsUdf(horz) )
		continue;

	    float polyz = avgz;
	    bool useavgz = true;
	    useavgz = !grdr.setGridPoint(coord) || !grdr.init();
	    if ( useavgz )
		polyz = avgz;
	    else
	    {
		polyz = grdr.getValue();
		if ( mIsUdf(polyz) )
		    polyz = avgz;
	    }

	    const float th = upw ? polyz - horz : horz - polyz;
	    if ( useneg || th > 0 )
		{ totth += th; break; }
	}
    }

    const float cellarea = SI().inlDistance() * hs.step.inl
			 * SI().crlDistance() * hs.step.crl;
    const float v = SI().zIsTime() ? vel * .5 : 1; // TWT
    return cellarea * v * totth;
}


const char* Poly2HorVol::dispText( float m3, bool zinft )
{
    static const float bblconv = 6.2898108;
    static const float ft3conv = 35.314667;

    if ( mIsUdf(m3) )
	return "";

    float dispval = m3;
    if ( zinft ) dispval *= ft3conv;
    bool mega = false;
    if ( fabs(dispval) > 1e6 )
	{ mega = true; dispval /= 1e6; }

    static BufferString txt;
    txt = dispval; txt += mega ? "M " : " ";
    txt += zinft ? "ft^3" : "m^3";
    txt += " (";
    dispval *= bblconv;
    if ( zinft ) dispval /= ft3conv;
    if ( dispval > 1e6 )
	{ mega = true; dispval /= 1e6; }
    txt += dispval; if ( mega ) txt += "M";
    txt += " bbl)";

    return txt.buf();
}
