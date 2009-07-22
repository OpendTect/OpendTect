/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID = "$Id: binidsurface.cc,v 1.19 2009-07-22 16:01:33 cvsbert Exp $";

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

#define mEstimateCorner( n1, ref, n2 ) \
(n1+n2-ref)


Coord3 BinIDSurface::computePosition( const Coord& param ) const
{
    const StepInterval<int> rowrange = rowRange();
    const StepInterval<int> colrange = colRange();

    int prevrowidx = rowrange.getIndex( param.x );
    if ( prevrowidx<0 || prevrowidx>nrRows()-1 )
	return Coord3::udf();
    else if ( prevrowidx>0 && prevrowidx==nrRows()-1 )
    {
	if ( rowrange.atIndex(prevrowidx)>=param.x )
	    prevrowidx--;
	else
	    return Coord3::udf();
    }

    int prevcolidx = colrange.getIndex(param.y);
    if ( prevcolidx<0 || prevcolidx>nrCols()-1 )
	return Coord3::udf();
    else if ( prevcolidx>0 && prevcolidx==nrCols()-1 )
    {
	if ( colrange.atIndex(prevcolidx)>=param.y )
	    prevcolidx--;
	else
	    return Coord3::udf();
    }

    float depth = mUdf(float);

    float depth00 = depths_->get( prevrowidx, prevcolidx );
    float depth01 = prevcolidx<nrCols()-1
	? depths_->get( prevrowidx, prevcolidx+1 )
	: mUdf(float);
    float depth10 = prevrowidx<nrRows()-1
	? depths_->get( prevrowidx+1, prevcolidx )
	: mUdf(float);
    float depth11 = prevcolidx<nrCols()-1 && prevrowidx<nrRows()-1
	? depths_->get( prevrowidx+1, prevcolidx+1 )
	: mUdf(float);

    const bool udef00 = mIsUdf( depth00 );
    const bool udef01 = mIsUdf( depth01 );
    const bool udef10 = mIsUdf( depth10 );
    const bool udef11 = mIsUdf( depth11 );

    const float u = rowrange.getfIndex(param.x)-prevrowidx;
    const float one_minus_u = 1-u;
    const float v = colrange.getfIndex(param.y)-prevcolidx;
    const float one_minus_v = 1-v;

    if ( mIsZero( u, 1e-3 ) )
    {
	if ( mIsZero( v, 1e-3 ) )
	    depth = depth00;
	else if ( mIsEqual( v, 1, 1e-3 ) )
	    depth = depth01;
	else if ( !udef00 && !udef01 )
	    depth = v * depth01+one_minus_v*depth00;
    }
    else if ( mIsEqual( u, 1, 1e-3 ) )
    {
	if ( mIsZero( v, 1e-3 ) )
	    depth = depth10;
	else if ( mIsEqual( v, 1, 1e-3 ) )
	    depth = depth11;
	else if ( !udef10 && !udef11 )
	    depth = v * depth11+one_minus_v*depth10;
    }
    else if ( mIsZero( v, 1e-3 ) && !udef00 && !udef10 )
	depth = u * depth10+one_minus_u*depth00;
    else if ( mIsEqual( v, 1, 1e-3 ) && !udef01 && !udef11 )
	depth = u * depth11+one_minus_u*depth01;

    if ( mIsUdf(depth) && nrRows() && nrCols() )
    {
	const char udefsum = udef00+udef01+udef10+udef11;
	if ( udefsum<=1 )
	{
	    if ( udefsum==1 )
	    {
		if ( udef00 )
		    depth00 = mEstimateCorner( depth10, depth11, depth01 );
		else if ( udef01 )
		    depth01 = mEstimateCorner( depth00, depth10, depth11 );
		else if ( udef10 )
		    depth10 = mEstimateCorner( depth00, depth01, depth11 );
		else //!def11
		    depth11 = mEstimateCorner( depth10, depth00, depth01 );
	    }

	    depth = (one_minus_u*depth00 + u*depth10) * one_minus_v +
		    (one_minus_u*depth01 + u*depth11) * v;
	}
    }

    return Coord3(SI().binID2Coord().transform(param), depth );
}


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


bool BinIDSurface::insertRow(int row, int nrtoinsert ) 
{
    mInsertStart( rowidx, row, nrRows() );
    mCloneRowVariable( float, depths_, computePosition(param).z, mUdf(float) )
    return true;
}


bool BinIDSurface::insertCol(int col, int nrtoinsert ) 
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

    const int nrremoved = stopidx-startidx+1;

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

    const int nrremoved = stopidx-startidx+1;

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


bool BinIDSurface::expandWithUdf( const RCol& start, const RCol& stop )
{
    if ( !depths_ ) 
	origin_ = start;

    const int oldnrrows = nrRows();
    const int oldnrcols = nrCols();

    int startrowidx = rowIndex( start.r() );
    startrowidx = startrowidx>=0 ? 0 : startrowidx;
    int startcolidx = colIndex( start.c() );
    startcolidx = startcolidx>=0 ? 0 : startcolidx;
    int stoprowidx = rowIndex( stop.r() );
    stoprowidx = stoprowidx<oldnrrows ? oldnrrows-1 : stoprowidx;
    int stopcolidx = colIndex( stop.c() );
    stopcolidx = stopcolidx<oldnrcols ? oldnrcols-1 : stopcolidx;
    
    const int newnrrows = stoprowidx-startrowidx+1;
    const int newnrcols = stopcolidx-startcolidx+1;

    if ( oldnrrows==newnrrows && oldnrcols==newnrcols )
	return true;
    
    Array2D<float>* newdepths = new Array2DImpl<float>( newnrrows, newnrcols );

    for ( int idx=0; newdepths && idx<newnrrows; idx++ )
    {
	for ( int idy=0; idy<newnrcols; idy++ )
	    newdepths->set( idx, idy, mUdf(float) );
    }

    for ( int idx=0; newdepths && idx<oldnrrows; idx++ )
    {
	for ( int idy=0; idy<oldnrcols; idy++ )
	{
	    newdepths->set( idx-startrowidx, idy-startcolidx, 
			    depths_->get( idx, idy ) );
	}
    }

    if ( newdepths ) 
    { 
	delete depths_; 
	depths_ = newdepths; 
    }
    
    origin_.row += step_.row*startrowidx;
    origin_.col += step_.col*startcolidx;

    return true;
}


Coord3 BinIDSurface::getKnot( const RCol& rc, bool interpolifudf ) const
{
    const int index = getKnotIndex( rc );
    if ( index<0 ) return Coord3::udf();

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

