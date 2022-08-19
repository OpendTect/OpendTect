/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
}


void Poly2HorVol::setHorizon( EM::Horizon3D* hor )
{
    hor_ = hor;
}


bool Poly2HorVol::setHorizon( const MultiID& mid, TaskRunner* tr )
{
    EM::EMObject* emobj = EM::EMM().loadIfNotFullyLoaded( mid, tr );
    mDynamicCastGet(EM::Horizon3D*,hor,emobj)

    setHorizon( hor );
    return hor_;
}


#define mPolyLoc(b) \
	Geom::Point2D<float>( (float)b.inl(), (float)(b.crl()) )

float Poly2HorVol::getM3( float vel, bool upw, bool useneg )
{
    if ( !hor_ || !ps_ || ps_->isEmpty() )
	return mUdf(float);

    ODPolygon<float> poly;
    TrcKeySampling polytks( false );
    TypeSet<Coord> pts;
    TypeSet<float> zvals;
    for ( int idx=0; idx<ps_->size(); idx++ )
    {
	const Pick::Location& pl = ps_->get( idx );
	pts += pl.pos();
	zvals += pl.z();
	const BinID bid( SI().transform(pl.pos()) );
	poly.add( mPolyLoc(bid) );
	polytks.include( bid );
    }

    TrcKeySampling tks = hor_->range();
    tks.shrinkTo( polytks );
    tks.expand( 1, 1 );

    TriangulatedGridder2D grdr;
    grdr.setPoints( pts );
    grdr.setValues( zvals );
    float avgz = 0.f;
    for ( int idx=0; idx<zvals.size(); idx++ )
	avgz += zvals[idx];

    if ( !zvals.isEmpty() )
	avgz /= sCast(float,zvals.size());

    TrcKeySamplingIterator iter( tks );
    BinID bid; float totth = 0;
    while ( iter.next(bid) )
    {
	if ( !poly.isInside(mPolyLoc(bid),true,1e-6) )
	    continue;

	const Coord pos( tks.toCoord(bid) );
	float horz = hor_->getZ( bid );
	if ( mIsUdf(horz) && bid.inl()!=tks.stop_.inl() &&
	     bid.crl()!=tks.stop_.crl() )
	{ //The very last edges should exclude.
	    horz = (float)hor_->geometry().geometryElement()->
						computePosition( pos ).z;
	}

	if ( mIsUdf(horz) )
	    continue;

	float polyz = grdr.getValue( pos );
	if ( mIsUdf(polyz) )
	    polyz = avgz;

	const float th = upw ? polyz - horz : horz - polyz;
	if ( useneg || th > 0 )
	    { totth += th; break; }
    }

    const float xyfactor = SI().xyInFeet() ? mFromFeetFactorF : 1;
    const float cellarea = SI().inlDistance() * tks.step_.inl() * xyfactor
			 * SI().crlDistance() * tks.step_.crl() * xyfactor;
    const float v = SI().zIsTime() ? vel * .5f : 1; // TWT
    if ( SI().zInFeet() )
	totth *= mFromFeetFactorF;

    return cellarea * v * totth;
}


const char* Poly2HorVol::dispText( float m3, bool inft )
{
    const float bblconv = 6.2898108;
    const float ft3conv = 35.314667;

    if ( mIsUdf(m3) )
	return "";

    float dispval = m3;
    if ( inft ) dispval *= ft3conv;
    bool mega = false;
    if ( fabs(dispval) > 1e6 )
	{ mega = true; dispval /= 1e6; }

    mDeclStaticString( txt );
    txt = dispval; txt += mega ? "M " : " ";
    txt += inft ? "ft^3" : "m^3";
    txt += " (";
    dispval = m3 * bblconv;
    if ( dispval > 1e6 )
	{ mega = true; dispval /= 1e6; }
    txt += dispval; if ( mega ) txt += "M";
    txt += " bbl)";

    return txt.buf();
}
