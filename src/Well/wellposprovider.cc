/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellposprovider.h"

#include "keystrs.h"
#include "iopar.h"
#include "horsampling.h"
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
    : hs_(*new HorSampling(true))
    , zrg_(SI().zRange(false))
    , onlysurfacecoords_(true)
{
}


WellProvider3D::WellProvider3D( const WellProvider3D& pp )
    : welldata_(pp.welldata_)
    , hs_(pp.hs_)
    , zrg_(pp.zrg_)
    , onlysurfacecoords_(pp.onlysurfacecoords_)
{
}


WellProvider3D::~WellProvider3D()
{
    for ( int idx=0; idx<wellids_.size(); idx++ )
	delete Well::MGR().release( wellids_[idx] );
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
    for ( int idx=0; idx<welldata_.size(); idx++ )
    {
	if ( onlysurfacecoords_ )
	{
	    const Well::Info& info = welldata_[idx]->info();
	    const BinID bid = SI().transform( info.surfacecoord );
	    hs_.include( bid );
	}
    }

    if ( !hs_.isDefined() )
	return;

    hs_.start.inl -= inlext_;
    hs_.stop.inl += inlext_;
    hs_.start.crl -= crlext_;
    hs_.stop.crl += crlext_;
}


bool WellProvider3D::initialize( TaskRunner* )
{
    welldata_.erase();
    for ( int idx=0; idx<wellids_.size(); idx++ )
	welldata_ += Well::MGR().get( wellids_[idx] );
    if ( welldata_.isEmpty() ) return false;

    setHS();
    curbid_ = hs_.start;
    if ( !toNextPos() )
	return false;

    curz_ = zrg_.stop;
    return true;
}


bool WellProvider3D::toNextPos()
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
    start = hs_.start; stop = hs_.stop;
}


void WellProvider3D::getZRange( Interval<float>& zrg ) const
{
    assign( zrg, zrg_ );
    mDynamicCastGet(StepInterval<float>*,szrg,&zrg)
    if ( szrg )
	szrg->step = zrg_.step;
}


od_int64 WellProvider3D::estNrPos() const
{ return hs_.totalNr(); }


const Well::Data* WellProvider3D::wellData( int idx ) const
{ return welldata_.validIdx(idx) ? welldata_[idx] : 0; }


void WellProvider3D::initClass()
{
    Provider3D::factory().addCreator( create, sKey::Well() );
}

} // namespace Pos
