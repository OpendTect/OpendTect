/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellposprovider.h"

#include "keystrs.h"
#include "iopar.h"
#include "survinfo.h"
#include "ioobj.h"
#include "ioman.h"
#include "ptrman.h"
#include "welldata.h"
#include "wellman.h"

namespace Pos
{

const char* WellProvider3D::sKeyInlExt()	{ return "Inline extension"; }
const char* WellProvider3D::sKeyCrlExt()	{ return "Crossline extension";}
const char* WellProvider3D::sKeyZExt()		{ return "Z extension"; }
const char* WellProvider3D::sKeySurfaceCoords() { return "Only surface coords";}


WellProvider3D::WellProvider3D()
    : hs_(*new TrcKeySampling(true))
    , zrg_(SI().zRange(false))
    , onlysurfacecoords_(true)
    , curwellidx_(0)
{
}


WellProvider3D::WellProvider3D( const WellProvider3D& pp )
    : welldata_(pp.welldata_)
    , hs_(pp.hs_)
    , zrg_(pp.zrg_)
    , onlysurfacecoords_(pp.onlysurfacecoords_)
    , curwellidx_(0)
{
}


WellProvider3D::~WellProvider3D()
{
    delete &hs_;
}


WellProvider3D& WellProvider3D::operator =( const WellProvider3D& pp )
{
    if ( &pp != this )
    {
	welldata_ = pp.welldata_;
	hs_ = pp.hs_;
	zrg_ = pp.zrg_;
    }
    return *this;
}


const char* WellProvider3D::type() const
{
    return sKey::Well();
}


void WellProvider3D::setHS()
{
    hs_.init( false );
    if ( onlysurfacecoords_ )
    {
	const Well::Info& info = welldata_[curwellidx_]->info();
	const BinID bid = SI().transform( info.surfacecoord_ );
	hs_.include( bid );
    }

    if ( !hs_.isDefined() )
	return;

    hs_.start_.inl() -= inlext_;
    hs_.stop_.inl() += inlext_;
    hs_.start_.crl() -= crlext_;
    hs_.stop_.crl() += crlext_;

    curbid_ = BinID::udf();
    hsitr_.setSampling( hs_ );
    hsitr_.next( curbid_ );
}


bool WellProvider3D::initialize( TaskRunner* )
{
    for ( int idx=0; idx<wellids_.size(); idx++ )
    {
	RefMan<Well::Data> wd = Well::MGR().get( wellids_[idx],
						 Well::LoadReqs(Well::Inf) );
	welldata_ += wd;
    }

    if ( welldata_.isEmpty() ) return false;

    setHS();
    curz_ = zrg_.start-zrg_.step;
    return true;
}


bool WellProvider3D::toNextWell()
{
    curwellidx_++;
    if ( !welldata_.validIdx(curwellidx_) )
	return false;
    setHS();
    return true;
}


bool WellProvider3D::toNextPos()
{
    curz_ = zrg_.start;

    if ( !hsitr_.next(curbid_) )
	return toNextWell();

    return true;
}


bool WellProvider3D::toNextZ()
{
    curz_ += zrg_.step;
    return curz_ > zrg_.stop + (1e-6*zrg_.step) ? toNextPos() : true;
}


bool WellProvider3D::includes( const BinID& bid, float z ) const
{
    if ( !hs_.includes(bid) )
	return false;

    if ( mIsUdf(z) ) return true;

    const float zeps = zrg_.step * 1e-6f;
    return z > zrg_.start - zeps && z < zrg_.stop + zeps;
}


bool WellProvider3D::includes( const Coord& c, float z ) const
{ return Provider3D::includes(c,z); }


#define mGetWellKey(k) IOPar::compKey(sKey::Well(),k)

void WellProvider3D::usePar( const IOPar& iop )
{
    iop.get( mGetWellKey(sKeyInlExt()), inlext_ );
    iop.get( mGetWellKey(sKeyCrlExt()), crlext_ );
    iop.get( mGetWellKey(sKeyZExt()), zext_ );
    iop.getYN( mGetWellKey(sKeySurfaceCoords()), onlysurfacecoords_ );
    int nrwells = 0;
    iop.get( mGetWellKey(sKey::Size()), nrwells );
    StepInterval<float> zrg;
    iop.get( sKey::ZRange(), zrg_ );
    for ( int idx=0; idx<nrwells; idx++ )
    {
	MultiID mid;
	BufferString idkey = IOPar::compKey( sKey::ID(), idx );
	iop.get( mGetWellKey(idkey), mid );
	wellids_ += mid;
    }

    initialize(0);
}


void WellProvider3D::fillPar( IOPar& iop ) const
{
    iop.set( mGetWellKey(sKeyInlExt()), inlext_ );
    iop.set( mGetWellKey(sKeyCrlExt()), crlext_ );
    iop.set( mGetWellKey(sKeyZExt()), zext_ );
    iop.setYN( mGetWellKey(sKeySurfaceCoords()), onlysurfacecoords_ );
    iop.set( mGetWellKey(sKey::Size()), wellids_.size() );
    iop.set( sKey::ZRange(), zrg_ );
    for ( int idx=0; idx<wellids_.size(); idx++ )
    {
	BufferString idkey = IOPar::compKey( sKey::ID(), idx );
	iop.set( mGetWellKey(idkey), wellids_[idx] );
    }
}


void WellProvider3D::getSummary( BufferString& txt ) const
{
    if ( wellids_.isEmpty() ) { txt += "No wells."; return; }
    txt.add( wellids_.size() ).add( " wells" );
}


void WellProvider3D::getExtent( BinID& start, BinID& stop ) const
{
    start = hs_.start_; stop = hs_.stop_;
}


void WellProvider3D::getZRange( Interval<float>& zrg ) const
{
    assign( zrg, zrg_ );
    mDynamicCastGet(StepInterval<float>*,szrg,&zrg)
    if ( szrg )
	szrg->step = zrg_.step;
}


od_int64 WellProvider3D::estNrPos() const
{ return welldata_.size() * hs_.totalNr(); }


ConstRefMan<Well::Data> WellProvider3D::wellData( int idx ) const
{ return welldata_.validIdx(idx) ? welldata_[idx] : nullptr; }


void WellProvider3D::initClass()
{
    Provider3D::factory().addCreator( create, sKey::Well() );
}

} // namespace Pos
