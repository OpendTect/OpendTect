/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID = "$Id: binidsurface.cc,v 1.9 2006-05-24 07:49:55 cvskris Exp $";

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
			     Array2D<float>* na, bool takeover )
{
    delete depths_;
    depths_ = takeover ? na : new Array2DImpl<float>( *na );
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


bool BinIDSurface::removeRow( int start, int stop )
{
    if ( start>stop )
	return false;

    const int curnrrows = nrRows();
    const int curnrcols = nrCols();

    const int startidx = rowIndex( start );
    const int stopidx = rowIndex( stop );
    if ( startidx<0 || startidx>=curnrrows || stopidx<0 || stopidx>=curnrrows )
    { 
	errmsg() = "Row to remove does not exist"; 
	return false; 
    }

    const int nrremoved = stop-start+1;

    Array2D<float>* newpositions = 
	depths_ ? new Array2DImpl<float>( curnrrows-nrremoved, curnrcols ) : 0;
    
    for ( int idx=0; newpositions && idx<curnrrows-nrremoved; idx++ )
    {
	for ( int idy=0; idy<curnrcols; idy++ )
	{
	    const int srcrow = idx<startidx ? idx : idx+nrremoved;
	    newpositions->set( idx, idy, depths_->get( srcrow, idy ) );
	}
    }

    if ( newpositions ) { delete depths_; depths_ = newpositions; }
    if ( !startidx )
	origin_.row += step_.row*nrremoved;
    
    return true;
}


bool BinIDSurface::removeCol( int start, int stop )
{
    const int curnrrows = nrRows();
    const int curnrcols = nrCols();

    const int startidx = colIndex( start );
    const int stopidx = colIndex( stop );
    if ( startidx<0 || startidx>=curnrcols || stopidx<0 || stopidx>=curnrcols )
    { 
	errmsg() = "Column to remove does not exist"; 
	return false; 
    }

    const int nrremoved = stop-start+1;

    Array2D<float>* newpositions = 
	depths_ ? new Array2DImpl<float>( curnrrows, curnrcols-nrremoved ) : 0;

    for ( int idx=0; newpositions && idx<curnrrows; idx++ )
    {
	for ( int idy=0; idy<curnrcols-nrremoved; idy++ )
	{
	    const int srccol = idy<startidx ? idy : idy+nrremoved;
	    newpositions->set( idx, idy, depths_->get( idx, srccol ) );
	}
    }
    if ( newpositions ) { delete depths_; depths_ = newpositions; }
    if ( !startidx )
	origin_.col += step_.col*nrremoved;

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

