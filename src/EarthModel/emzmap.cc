/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emzmap.h"

#include "arrayndimpl.h"
#include "od_istream.h"
#include "separstr.h"
#include "survinfo.h"
#include "unitofmeasure.h"


namespace EM
{

ZMapImporter::ZMapImporter( const char* fnm )
    : Executor("Reading ZMap data")
    , fnm_(fnm)
{
    nrdonetxt_ = tr("Positions done");
    istrm_ = new od_istream( fnm_ );

    initHeader();
    applyCRS();
}


ZMapImporter::~ZMapImporter()
{
    delete istrm_;
    delete data_;
    delete uom_;
}


void ZMapImporter::setCoordSystem( Coords::CoordSystem* crs )
{
    coordsystem_ = crs;
    applyCRS();
}


void ZMapImporter::setUOM( const UnitOfMeasure* uom )
{
    deleteAndZeroPtr( uom_ );
    if ( uom )
	uom_ = new UnitOfMeasure( *uom );
}


bool ZMapImporter::initHeader()
{
    BufferString buf;
    while( true ) // comments
    {
	istrm_->getLine( buf );
	if ( buf.isEmpty() || buf[0]=='!' )
	    continue;

	break;
    }

// header line 1
    SeparString ss( buf, ',' );
    if ( ss.size() != 3 )
	return false;
    nrnodesperline_ = toInt( ss[2] );

//header line 2
    istrm_->getLine( buf ); ss.set( buf );
    if ( ss.size() != 5 )
	return false;
    nrchars_ = toInt( ss[0] );
    undefval_ = toFloat( ss[1] );
    undeftxt_ = ss[2];
    nrdec_ = toInt( ss[3] );
    firstcol_ = toInt( ss[4] );

//header line 3
    istrm_->getLine( buf ); ss.set( buf );
    if ( ss.size() != 6 )
	return false;
    nrrows_ = toInt( ss[0] );
    nrcols_ = toInt( ss[1] );
    xmin_ = toDouble( ss[2] );
    xmax_ = toDouble( ss[3] );
    ymin_ = toDouble( ss[4] );
    ymax_ = toDouble( ss[5] );

//header line 4
    istrm_->getLine( buf );

// end
    istrm_->getLine( buf );

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
    if ( !data_ )
    {
	data_ = new Array2DImpl<float>( nrrows_, nrcols_ );
	data_->setAll( mUdf(float) );
    }

    if ( nrdone_ >= totalnr_ )
	return Finished();

    BufferString buf;
    istrm_->getWord( buf );
    float zval = toFloat( buf );
    if ( mIsUdf(zval) || mIsEqual(zval,undefval_,mDefEps) )
    {
	nrdone_++;
	return MoreToDo();
    }

    if ( uom_ )
	zval = uom_->internalValue( zval );

    // From ZMap format description:
    // grid nodes are stored in column major order. The first column of
    // data is written first, starting at the upper left corner of the grid.
    const int row = (nrrows_-1) - (int)(nrdone_ % nrrows_);
    const int col = (int)(nrdone_ / nrrows_);
    data_->set( row, col, zval );

    nrdone_++;
    return MoreToDo();
}


Coord ZMapImporter::minCoord() const
{ return mincrd_; }

Coord ZMapImporter::maxCoord() const
{ return maxcrd_; }

Coord ZMapImporter::step() const
{ return Coord( dx_, dy_ ); }

} // namespace EM
