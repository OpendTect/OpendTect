/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2005
-*/


#include "polyposprovider.h"
#include "keystrs.h"
#include "iopar.h"
#include "polygon.h"
#include "trckeyzsampling.h"
#include "survinfo.h"
#include "ioobj.h"
#include "ioman.h"
#include "ptrman.h"
#include "picksetmanager.h"
#include <math.h>


Pos::PolyProvider3D::PolyProvider3D()
    : poly_(*new ODPolygon<float>)
    , hs_(*new TrcKeySampling(true))
    , zrg_(SI().zRange(false))
{
}


Pos::PolyProvider3D::PolyProvider3D( const Pos::PolyProvider3D& pp )
    : poly_(*new ODPolygon<float>(pp.poly_))
    , hs_(pp.hs_)
    , zrg_(pp.zrg_)
    , mid_ (pp.mid_)
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
	mid_ = pp.mid_;
    }
    return *this;
}


const char* Pos::PolyProvider3D::type() const
{
    return sKey::Polygon();
}


static void setHS( const ODPolygon<float>& poly, TrcKeySampling& hs )
{
    if ( poly.size() < 2 )
	{ hs = SI().sampling(true).hsamp_; return; }

    const Interval<float> xrg( poly.getRange(true) );
    const Interval<float> yrg( poly.getRange(false) );
    hs.start_.inl() = (int)Math::Floor( xrg.start + 0.5 );
    hs.start_.crl() = (int)Math::Floor( yrg.start + 0.5 );
    hs.stop_.inl() = (int)Math::Ceil( xrg.stop - 0.5 );
    hs.stop_.crl() = (int)Math::Ceil( yrg.stop - 0.5 );
    SI().snap( hs.start_, BinID(1,1) );
    SI().snap( hs.stop_, BinID(-1,-1) );
}


bool Pos::PolyProvider3D::initialize( TaskRunner* )
{
    if ( poly_.size() < 2 ) return false;

    setHS( poly_, hs_ );
    curbid_ = hs_.start_;
    if ( !toNextPos() )
	return false;

    curbid_.crl() -= hs_.step_.crl();
    curz_ = zrg_.stop;
    return true;
}


bool Pos::PolyProvider3D::toNextPos()
{
    curbid_.crl() += hs_.step_.crl();
    curz_ = zrg_.start;

    while ( true )
    {
	if ( !hs_.includes(curbid_) )
	{
	    curbid_.inl() += hs_.step_.inl();
	    curbid_.crl() = hs_.start_.crl();
	    if ( !hs_.includes(curbid_) )
		break;
	}
	if ( includes(curbid_,mUdf(float)) )
	    return true;
	curbid_.crl() += hs_.step_.crl();
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
    if ( !poly_.isInside( Geom::Point2D<float>(mCast(float,bid.inl()),
					mCast(float,bid.crl())),true,mDefEps ) )
	return false;

    if ( mIsUdf(z) ) return true;

    const float zeps = zrg_.step * 1e-6f;
    return z > zrg_.start - zeps && z < zrg_.stop + zeps;
}


#define mGetPolyKey(k) IOPar::compKey(sKey::Polygon(),k)

ODPolygon<float>* Pos::PolyProvider3D::polyFromPar( const IOPar& iop, int nr )
{
    const char* res = iop.find( mGetPolyKey("ID") );
    ODPolygon<float>* ret = 0;
    if ( res && *res )
    {
	ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch(
						DBKey::getFromString(res) );
	if ( ps )
	{
	    ret = new ODPolygon<float>;
	    ps->getPolygon( *ret );
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
    iop.get( mGetPolyKey(sKey::ZRange()), zrg_ );
    iop.get( mGetPolyKey(sKey::StepInl()), hs_.step_.inl() );
    iop.get( mGetPolyKey(sKey::StepCrl()), hs_.step_.crl() );
    iop.get( mGetPolyKey(sKey::ID()), mid_ );
    ODPolygon<float>* poly = polyFromPar( iop );
    if ( poly )
    {
	poly_ = *poly;
	setHS( poly_, hs_ );
    }
}


void Pos::PolyProvider3D::fillPar( IOPar& iop ) const
{
    iop.set( mGetPolyKey(sKey::ZRange()), zrg_ );
    iop.set( mGetPolyKey(sKey::StepInl()), hs_.step_.inl() );
    iop.set( mGetPolyKey(sKey::StepCrl()), hs_.step_.crl() );
    iop.set( mGetPolyKey(sKey::ID()), mid_ );
    ::fillPar( iop, poly_, mGetPolyKey(((int)0)) );
}


void Pos::PolyProvider3D::getSummary( BufferString& txt ) const
{
    if ( poly_.isEmpty() )
	{ txt += "No points. Unsaved?"; return; }

    txt.add( "area " ).add( hs_.start_.toString() );
    txt.add( "-" ).add( hs_.stop_.toString() );
    const int nrsamps = zrg_.nrSteps() + 1;
    if ( nrsamps > 1 )
	txt.add( " (" ).add( nrsamps ).add( " samples)" );
}


void Pos::PolyProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    start = hs_.start_; stop = hs_.stop_;
}


void Pos::PolyProvider3D::getZRange( Interval<float>& zrg ) const
{
    assign( zrg, zrg_ );
    mDynamicCastGet(StepInterval<float>*,szrg,&zrg)
    if ( szrg )
	szrg->step = zrg_.step;
}


od_int64 Pos::PolyProvider3D::estNrPos() const
{
    float fnr = (float) poly_.area() / hs_.step_.inl();
    fnr /= hs_.step_.crl();
    return mRounded(od_int64,fnr);
}


void Pos::PolyProvider3D::initClass()
{
    Pos::Provider3D::factory().addCreator( create, sKey::Polygon() );
}
