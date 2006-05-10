/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID = "$Id: binidsurface.cc,v 1.7 2006-05-10 15:23:07 cvskris Exp $";

#include "binidsurface.h"

#include "parametricsurfaceimpl.h"

#include "arrayndimpl.h"
#include "errh.h"
#include "rowcol.h"
#include "survinfo.h"


namespace Geometry
{


BinIDSurface::BinIDSurface( const RCol& newstep )
    : ParametricSurface( RowCol(0,0), newstep ) 
    , depths_( 0 )
{ }


BinIDSurface::BinIDSurface( const BinIDSurface& b )
    : ParametricSurface( b.origin_, b.step_ ) 
    , depths_( b.depths_ ? new Array2DImpl<float>(*b.depths_) : 0 )
{ }


BinIDSurface::~BinIDSurface()
{ delete depths_; }


BinIDSurface* BinIDSurface::clone() const
{ return new BinIDSurface(*this); }


void BinIDSurface::setArray( const RCol& start, const RCol& step,
			     Array2D<float>* na )
{
    delete depths_;
    depths_ = na;
    origin_ = start;
    step_ = step;
}


bool BinIDSurface::insertRow(int row) 
{
    mInsertStart( rowidx, row, nrRows() );
    mCloneRowVariable( float, depths_, computePosition(param).z, mUdf(float) )
    return true;
}


bool BinIDSurface::insertCol(int col) 
{
    mInsertStart( colidx, col, nrCols() );
    mCloneColVariable( float, depths_, computePosition(param).z, mUdf(float) )
    return true;
}


bool BinIDSurface::removeRow( int row )
{
    pErrMsg( "not implemented ");
    return true;
}


bool BinIDSurface::removeCol( int col )
{
    pErrMsg( "not implemented ");
    return true;
}


Coord3 BinIDSurface::getKnot( const RCol& rc, bool interpolifudf ) const
{
    const int index = getKnotIndex(rc);
    if ( index==-1 ) return Coord3::udf();

    return Coord3(SI().transform(BinID(rc)), depths_->getData()[index]);
}


void BinIDSurface::_setKnot( int idx, const Coord3& np )
{
    if ( !depths_ )
    {
	depths_ = new Array2DImpl<float>( 1, 1 );
	idx = 0;
    }

    depths_->getData()[idx] = np.z;
}


int BinIDSurface::nrRows() const
{ return depths_ ? depths_->info().getSize(rowDim()) : 0; }


int BinIDSurface::nrCols() const
{ return depths_ ? depths_->info().getSize(colDim()) : 0; }


};

