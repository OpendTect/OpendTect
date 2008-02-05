/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/

static const char* rcsID = "$Id: polyposprovider.cc,v 1.1 2008-02-05 14:25:26 cvsbert Exp $";

#include "polyposprovider.h"
#include "keystrs.h"
#include "iopar.h"
#include "polygon.h"
#include "cubesampling.h"
#include "survinfo.h"
#include "ioobj.h"
#include "ioman.h"
#include "ptrman.h"
#include "picksettr.h"
#include <math.h>


Pos::PolyProvider3D::PolyProvider3D()
    : poly_(*new ODPolygon<float>)
    , hs_(*new HorSampling(true))
    , zrg_(SI().zRange(false))
{
}


Pos::PolyProvider3D::PolyProvider3D( const PolyProvider3D::PolyProvider3D& pp )
    : poly_(*new ODPolygon<float>(pp.poly_))
    , hs_(pp.hs_)
    , zrg_(pp.zrg_)
{
}


Pos::PolyProvider3D::~PolyProvider3D()
{
    delete &poly_;
    delete &hs_;
}


Pos::PolyProvider3D& Pos::PolyProvider3D::operator =( const PolyProvider3D& pp )
{
    if ( &pp != this )
    {
	poly_ = pp.poly_;
	hs_ = pp.hs_;
	zrg_ = pp.zrg_;
    }
    return *this;
}


bool Pos::PolyProvider3D::initialize()
{
    const int nrvtx = poly_.size();
    if ( nrvtx < 2 ) return false;

    const Interval<float> xrg( poly_.getRange(true) );
    const Interval<float> yrg( poly_.getRange(false) );
#define mSetStart(xy,ic) \
    hs_.start.ic = (int)ceil( xy##rg.start - 1e-6 ); \
    if ( hs_.start.ic % hs_.step.ic ) \
	hs_.start.ic += hs_.step.ic - (hs_.start.ic % hs_.step.ic)
    mSetStart(x,inl);
    mSetStart(y,crl);
#define mSetStop(xy,ic) \
    hs_.stop.ic = (int)floor( xy##rg.stop + 1e-6 ); \
    if ( hs_.stop.ic % hs_.step.ic ) \
	hs_.stop.ic -= hs_.step.ic - (hs_.stop.ic % hs_.step.ic)
    mSetStop(x,inl);
    mSetStop(y,crl);

    curbid_ = hs_.start;
    if ( !toNextPos() )
	return false;

    curbid_.crl -= hs_.step.crl;
    curz_ = zrg_.stop;
    return true;
}


bool Pos::PolyProvider3D::toNextPos()
{
    curbid_.crl += hs_.step.crl;
    curz_ = zrg_.start;

    while ( true )
    {
	if ( !hs_.includes(curbid_) )
	{
	    curbid_.inl += hs_.step.inl;
	    curbid_.crl = hs_.start.crl;
	    if ( !hs_.includes(curbid_) )
		break;
	}
	if ( includes(curbid_,mUdf(float)) )
	    return true;
	curbid_.crl += hs_.step.crl;
    }

    return false;
}


bool Pos::PolyProvider3D::toNextZ()
{
    curz_ += zrg_.step;
    return curz_ > zrg_.stop + (1e-6*zrg_.step) ? toNextPos() : true;
}


bool Pos::PolyProvider3D::includes( const BinID& bid, float z ) const
{
    if ( !poly_.isInside(Geom::Point2D<float>(bid.inl,bid.crl),true,mDefEps ) )
	return false;

    if ( mIsUdf(z) ) return true;

    const float zeps = zrg_.step * 1e-6;
    return z > zrg_.start - zeps && z < zrg_.stop + zeps;
}


#define mGetPolyKey(k) IOPar::compKey(sKey::Polygon,k)

ODPolygon<float>* Pos::PolyProvider3D::polyFromPar( const IOPar& iop, int nr )
{
    const char* res = iop.find( mGetPolyKey("ID") );
    if ( !res || !*res )
	res = iop.find( "ID" );

    ODPolygon<float>* ret = 0;
    if ( res && *res )
    {
	PtrMan<IOObj> ioobj = IOM().get( res );
	if ( ioobj )
	{
	    BufferString msg;
	    ret = PickSetTranslator::getPolygon(*ioobj,msg);
	}
    }

    if ( !ret )
    {
	ret = new ODPolygon<float>;
	::usePar( iop, *ret, mGetPolyKey(nr) );
	if ( ret->size() < 2 )
	    { delete ret; ret = 0; }
    }

    return ret;
}


void Pos::PolyProvider3D::usePar( const IOPar& iop )
{
    iop.get( mGetPolyKey(sKey::ZRange), zrg_ );
    iop.get( mGetPolyKey("Steps"), hs_.step );
    ODPolygon<float>* poly = polyFromPar( iop );
    if ( poly ) poly_ = *poly;
}


void Pos::PolyProvider3D::fillPar( IOPar& iop ) const
{
    iop.set( mGetPolyKey(sKey::ZRange), zrg_ );
    iop.set( mGetPolyKey("Steps"), hs_.step );
    ::fillPar( iop, poly_, mGetPolyKey(((int)0)) );
}


void Pos::PolyProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    start = hs_.start; stop = hs_.stop;
}


void Pos::PolyProvider3D::initClass()
{
    Pos::Provider3D::factory().addCreator( create, "Polygon" );
}
