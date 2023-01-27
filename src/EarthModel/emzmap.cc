/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2014
________________________________________________________________________

-*/


#include "emzmap.h"

#include "arrayndimpl.h"
#include "hiddenparam.h"
#include "od_istream.h"
#include "posinfodetector.h"
#include "separstr.h"
#include "survinfo.h"
#include "unitofmeasure.h"

#include "uistrings.h"


namespace EM
{
static HiddenParam<ZMapImporter,UnitOfMeasure*> uoms_(nullptr);
static HiddenParam<ZMapImporter,PosInfo::Detector*> posds_(nullptr);

ZMapImporter::ZMapImporter( const char* fnm )
    : Executor("Reading ZMap data")
    , fnm_(fnm)
{
    nrdonetxt_ = tr("Positions done");
    istrm_ = new od_istream( fnm_ );

    uoms_.setParam( this, nullptr );
    posds_.setParam( this, new PosInfo::Detector(false) );
    initHeader();
    applyCRS();
}


ZMapImporter::~ZMapImporter()
{
    delete istrm_;
    delete data_;
    uoms_.removeAndDeleteParam( this );
    posds_.removeAndDeleteParam( this );
}


void ZMapImporter::setCoordSystem( Coords::CoordSystem* crs )
{
    coordsystem_ = crs;
    applyCRS();
}


void ZMapImporter::setUOM( const UnitOfMeasure* uom )
{
    uoms_.deleteAndZeroPtrParam( this );
    if ( uom )
	uoms_.setParam( this, new UnitOfMeasure(*uom) );
}


bool ZMapImporter::initHeader()
{
    BufferString linestr;
    while( true ) // comments
    {
	istrm_->getLine( linestr );
	if ( linestr.isEmpty() || linestr[0]=='!' )
	    continue;

	break;
    }

// header line 1
    SeparStringSB ss( linestr, ',' );
    if ( ss.size() != 3 )
	return false;
    nrnodesperline_ = toInt( ss[2] );

//header line 2
    istrm_->getLine( linestr );
    ss = linestr.buf();
    if ( ss.size() != 5 )
	return false;
    nrchars_ = toInt( ss[0] );
    undefval_ = toFloat( ss[1] );
    undeftxt_ = ss[2];
    nrdec_ = toInt( ss[3] );
    firstcol_ = toInt( ss[4] );

//header line 3
    istrm_->getLine( linestr );
    ss = linestr.buf();
    if ( ss.size() != 6 )
	return false;
    nrrows_ = toInt( ss[0] );
    nrcols_ = toInt( ss[1] );
    xmin_ = toDouble( ss[2] );
    xmax_ = toDouble( ss[3] );
    ymin_ = toDouble( ss[4] );
    ymax_ = toDouble( ss[5] );

//header line 4
    istrm_->getLine( linestr );

// end
    istrm_->getLine( linestr );

    totalnr_ = nrrows_ * nrcols_;
    return true;
}


void ZMapImporter::applyCRS()
{
    mincrd_ = Coord( xmin_, ymin_ );
    maxcrd_ = Coord( xmax_, ymax_ );

    const Coords::CoordSystem* fromcrs = coordsystem_;
    const Coords::CoordSystem* tocrs = SI().getCoordSystem();
    if ( fromcrs && tocrs )
    {
	mincrd_ = Coords::CoordSystem::convert( mincrd_, *fromcrs, *tocrs );
	maxcrd_ = Coords::CoordSystem::convert( maxcrd_, *fromcrs, *tocrs );
    }

    const Coord dcrd = maxcrd_ - mincrd_;
    dx_ = dcrd.x / (nrcols_-1);
    dy_ = dcrd.y / (nrrows_-1);
}


int ZMapImporter::nextStep()
{
    if ( totalnr_ == 0 )
	return ErrorOccurred();

    if ( !data_ )
    {
	data_ = new Array2DImpl<float>( nrrows_, nrcols_ );
	data_->setAll( mUdf(float) );
    }

    auto* posdetector = posds_.getParam( this );
    if ( nrdone_ >= totalnr_ )
    {
	posdetector->finish();
	return Finished();
    }

    BufferString buf;
    istrm_->getWord( buf );
    float zval = toFloat( buf );
    if ( mIsUdf(zval) || mIsEqual(zval,undefval_,mDefEps) )
    {
	nrdone_++;
	return MoreToDo();
    }

    UnitOfMeasure* uom = uoms_.getParam( this );
    if ( uom )
	zval = uom->internalValue( zval );

    // From ZMap format description:
    // grid nodes are stored in column major order. The first column of
    // data is written first, starting at the upper left corner of the grid.
    const int row = (nrrows_-1) - (int)(nrdone_ % nrrows_);
    const int col = (int)(nrdone_ / nrrows_);
    data_->set( row, col, zval );

    Coord curcrd;
    curcrd.x = mincrd_.x + dx_*col;
    curcrd.y = mincrd_.y + dy_*row;
    const BinID curbid = SI().transform( curcrd );
    posdetector->add( curcrd, curbid );

    nrdone_++;
    return MoreToDo();
}


Coord ZMapImporter::minCoord() const
{ return mincrd_; }

Coord ZMapImporter::maxCoord() const
{ return maxcrd_; }

Coord ZMapImporter::step() const
{ return Coord( dx_, dy_ ); }


TrcKeySampling ZMapImporter::sampling() const
{
    TrcKeySampling tks( false );
    tks.include( TrcKey(SI().transform(mincrd_)) );
    tks.include( TrcKey(SI().transform(maxcrd_)) );
    tks.include( TrcKey(SI().transform(Coord(mincrd_.x,maxcrd_.y))) );
    tks.include( TrcKey(SI().transform(Coord(maxcrd_.x,mincrd_.y))) );

    auto* posdetector = posds_.getParam( this );
    tks.step_ = BinID( Math::Abs(posdetector->step().inl()),
		       Math::Abs(posdetector->step().crl()) );
    return tks;
}


const PosInfo::Detector& ZMapImporter::detector() const
{
    return *posds_.getParam( this );
}

} // namespace EM
