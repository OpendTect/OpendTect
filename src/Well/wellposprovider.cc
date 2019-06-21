/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2012
________________________________________________________________________

-*/


#include "wellposprovider.h"

#include "keystrs.h"
#include "iopar.h"
#include "survinfo.h"
#include "ioobj.h"
#include "ptrman.h"
#include "taskrunner.h"
#include "welldata.h"
#include "wellinfo.h"
#include "wellmanager.h"
#include "uistrings.h"

namespace Pos
{

const char* WellProvider3D::sKeyInlExt()	{ return "Inline extension"; }
const char* WellProvider3D::sKeyCrlExt()	{ return "Crossline extension";}
const char* WellProvider3D::sKeyZExt()		{ return "Z extension"; }
const char* WellProvider3D::sKeySurfaceCoords() { return "Only surface coords";}


WellProvider3D::WellProvider3D()
    : Pos::Provider3D()
    , hs_(*new TrcKeySampling(true))
    , zrg_(SI().zRange())
    , onlysurfacecoords_(true)
    , curwellidx_(0)
{
}


WellProvider3D::WellProvider3D( const WellProvider3D& oth )
    : Pos::Provider3D(oth)
    , hs_(*new TrcKeySampling(oth.hs_))
{
    *this = oth;
}


WellProvider3D::~WellProvider3D()
{
    delete &hs_;
}


WellProvider3D& WellProvider3D::operator =( const WellProvider3D& oth )
{
    if ( &oth == this )
	return *this;

    Pos::Provider3D::operator = ( oth );

    wellids_ = oth.wellids_;
    welldata_ = oth.welldata_;
    onlysurfacecoords_ = oth.onlysurfacecoords_;
    inlext_ = oth.inlext_;
    crlext_ = oth.crlext_;
    zext_ = oth.zext_;
    hs_ = oth.hs_;
    hsitr_.setSampling( hs_ );
    hsitr_.setCurrentPos( oth.hsitr_.curIdx() );
    zrg_ = oth.zrg_;
    curbid_ = oth.curbid_;
    curz_ = oth.curz_;
    curwellidx_ = oth.curwellidx_;

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
	const BinID bid = SI().transform( info.surfaceCoord() );
	hs_.include( bid );
    }

    if ( !hs_.isDefined() )
	return;

    hs_.start_.inl() -= inlext_;
    hs_.stop_.inl() += inlext_;
    hs_.start_.crl() -= crlext_;
    hs_.stop_.crl() += crlext_;

    hsitr_.setSampling( hs_ );
    curbid_ = hsitr_.curBinID();
    hsitr_.next();
}


void WellProvider3D::reset()
{
    SilentTaskRunnerProvider trprov;
    initialize( trprov );
}


bool WellProvider3D::initialize( const TaskRunnerProvider& trprov )
{
    welldata_.setEmpty();
    for ( int idx=0; idx<wellids_.size(); idx++ )
    {
	ConstRefMan<Well::Data> wd = Well::MGR().fetch( wellids_[idx],
					    Well::LoadReqs(Well::Trck) );
	welldata_ += wd;
    }

    if ( welldata_.isEmpty() )
	return false;

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
    curbid_ = hsitr_.curBinID();
    curz_ = zrg_.start;

    return hsitr_.next() ? true : toNextWell();
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
	DBKey mid;
	BufferString idkey = IOPar::compKey( sKey::ID(), idx );
	iop.get( mGetWellKey(idkey), mid );
	wellids_ += mid;
    }

    reset();
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


void WellProvider3D::getSummary( uiString& txt ) const
{
    if ( wellids_.isEmpty() )
    {
	txt.appendPhrase(tr("No wells present"), uiString::Space,
						uiString::OnSameLine);
	return;
    }
    txt.appendPhrase( toUiString("%1 %2").arg(wellids_.size())
			.arg(uiStrings::sWell(wellids_.size()).toLower()) );
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
{
    return welldata_.size() * hs_.totalNr();
}


const Well::Data* WellProvider3D::wellData( int idx ) const
{
    return welldata_.validIdx(idx) ? welldata_[idx] : 0;
}


void WellProvider3D::initClass()
{
    Provider3D::factory().addCreator( create, sKey::Well(),
							uiStrings::sWell() );
}

} // namespace Pos
