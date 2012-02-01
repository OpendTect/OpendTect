/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
________________________________________________________________________

-*/

static const char* rcsID = "$Id: wellposprovider.cc,v 1.2 2012-02-01 23:29:55 cvsnanne Exp $";

#include "wellposprovider.h"

#include "keystrs.h"
#include "iopar.h"
#include "cubesampling.h"
#include "survinfo.h"
#include "ioobj.h"
#include "ioman.h"
#include "ptrman.h"
#include "welldata.h"
#include "wellman.h"

#include <math.h>

namespace Pos
{

const char* WellProvider3D::sKeyInlExt()	{ return "Inline extension"; }
const char* WellProvider3D::sKeyCrExt()		{ return "Crossline extension";}
const char* WellProvider3D::sKeyZExt()		{ return "Z extension"; }
const char* WellProvider3D::sKeySurfaceCoords() { return "Only surface coords";}


WellProvider3D::WellProvider3D()
    : hs_(*new HorSampling(true))
    , zrg_(SI().zRange(false))
{
}


WellProvider3D::WellProvider3D( const WellProvider3D& pp )
    : welldata_(pp.welldata_)
    , hs_(pp.hs_)
    , zrg_(pp.zrg_)
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
    return sKey::Well;
}


static void setHS( const Well::Data& wd, HorSampling& hs )
{
    const Well::Track& track = wd.track();

 /*   if ( poly.size() < 2 )
	{ hs = SI().sampling(true).hrg; return; }

    const Interval<float> xrg( poly.getRange(true) );
    const Interval<float> yrg( poly.getRange(false) );*/
    hs.start.inl = (int)floor( xrg.start + 0.5 );
    hs.start.crl = (int)floor( yrg.start + 0.5 );
    hs.stop.inl = (int)floor( xrg.stop + 0.5 );
    hs.stop.crl = (int)floor( yrg.stop + 0.5 );
    SI().snap( hs.start, 1 );
    SI().snap( hs.stop, -1 );
}


bool WellProvider3D::initialize( TaskRunner* )
{
    for ( int idx=0; idx<wellids_.size(); idx++ )
	welldata_ += Well::MGR().get( wellids_[idx] );
    if ( welldata_.isEmpty() ) return false;

    setHS( *welldata_, hs_ );
    curbid_ = hs_.start;
    if ( !toNextPos() )
	return false;

    curbid_.crl -= hs_.step.crl;
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

    const float zeps = zrg_.step * 1e-6;
    return z > zrg_.start - zeps && z < zrg_.stop + zeps;
}


bool WellProvider3D::includes( const Coord& c, float z ) const
{ return Provider3D::includes(c,z); }


#define mGetWellKey(k) IOPar::compKey(sKey::Well,k)

void WellProvider3D::usePar( const IOPar& iop )
{
    iop.get( mGetWellKey("Inline extension"), inlext_ );
    iop.get( mGetWellKey("Crossline extension"), crlext_ );
    iop.get( mGetWellKey("Z extension"), zext_ );
    iop.get( mGetWellKey("Only surface coords"), onlysurfacecoords_ );



    iop.get( mGetWellKey(sKey::StepInl), hs_.step.inl );
    iop.get( mGetWellKey(sKey::StepCrl), hs_.step.crl );
    ODPolygon<float>* poly = polyFromPar( iop );
    if ( poly )
    {
	poly_ = *poly;
	setHS( poly_, hs_ );
    }
}


void WellProvider3D::fillPar( IOPar& iop ) const
{
    iop.set( mGetWellKey(sKey::ZRange), zrg_ );
    iop.set( mGetWellKey(sKey::StepInl), hs_.step.inl );
    iop.set( mGetWellKey(sKey::StepCrl), hs_.step.crl );
    ::fillPar( iop, poly_, mGetPolyKey(((int)0)) );
}


void WellProvider3D::getSummary( BufferString& txt ) const
{
    if ( poly_.isEmpty() ) { txt += "No points. Unsaved?"; return; }
    txt += "area "; BufferString tmp;
    hs_.start.fill( tmp.buf() ); txt += tmp; txt += "-";
    hs_.stop.fill( tmp.buf() ); txt += tmp;
    const int nrsamps = zrg_.nrSteps() + 1;
    if ( nrsamps > 1 )
	{ txt += " ("; txt += nrsamps; txt += " samples)"; }
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
{
    float fnr = poly_.area() / hs_.step.inl;
    fnr /= hs_.step.crl;
    return mRounded(od_int64,fnr);
}


const Well::Data* WellProvider3D::wellData( int idx ) const
{ return welldata_.validIdx(idx) ? welldata_[idx] : 0; }


void WellProvider3D::initClass()
{
    Provider3D::factory().addCreator( create, sKey::Well );
}

} // namespace Pos
