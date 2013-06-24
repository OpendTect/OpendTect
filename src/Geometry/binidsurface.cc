/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "binidsurface.h"

#include "parametricsurfaceimpl.h"

#include "arrayndimpl.h"
#include "errh.h"
#include "rowcol.h"
#include "survinfo.h"
#include "trigonometry.h"


namespace Geometry
{


BinIDSurface::BinIDSurface( const BinID& newstep )
    : ParametricSurface( RowCol(0,0), RowCol(newstep) ) 
    , depths_( 0 )
    , surveyinfo_( &SI() )
{ }


BinIDSurface::BinIDSurface( const BinIDSurface& b )
    : ParametricSurface( b.origin_, b.step_ ) 
    , depths_( b.depths_ ? new Array2DImpl<float>(*b.depths_) : 0 )
    , surveyinfo_( &SI() )
{ }


BinIDSurface::~BinIDSurface()
{ delete depths_; }

#define mEstimateCorner( n1, ref, n2 ) \
(n1+n2-ref)


Coord3 BinIDSurface::computePosition( const Coord& param ) const
{
    if ( !depths_ )
	return Coord3::udf();

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


void BinIDSurface::setArray( const BinID& start, const BinID& step,
			     Array2D<float>* na, bool takeover )
{
    bool ismovement = false;
    if ( depths_ && na )
    {
	if ( depths_->info().getSize(0)==na->info().getSize(0) &&
	     depths_->info().getSize(1)==na->info().getSize(1) &&
	     step == step_ && origin_==start )
	    ismovement = true;
    }

    delete depths_;
    depths_ = takeover ? na : new Array2DImpl<float>( *na );
    origin_ = RowCol(start);
    step_ = RowCol(step);

    if ( ismovement )
	triggerMovement();
    else
	triggerNrPosCh();
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

    Array2D<float>* newpositions =  0;
    if ( depths_ )
    {
	mTryAlloc( newpositions,
		   Array2DImpl<float>( curnrrows-nrremoved, curnrcols ) );
	if ( !newpositions || !newpositions->isOK() )
	{
	    delete newpositions;
	    return false; //out of memory
	}
    }
    
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

    Array2D<float>* newpositions =  0;
    if ( depths_ )
    {
	mTryAlloc( newpositions,
		   Array2DImpl<float>( curnrrows, curnrcols-nrremoved ) );
	if ( !newpositions || !newpositions->isOK() )
	{
	    delete newpositions;
	    return false; //out of memory
	}
    }

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


StepInterval<int> BinIDSurface::rowRange() const
{
    return ParametricSurface::rowRange();
}


StepInterval<int> BinIDSurface::colRange() const
{
    return ParametricSurface::colRange();
}


StepInterval<int> BinIDSurface::rowRange( int col ) const
{
    StepInterval<int> ret( mUdf(int), mUdf(int), step_.row );
    const int colidx = colIndex( col );
    if ( colidx < 0  || !depths_ || colidx >= depths_->info().getSize(1) )
	return ret;

    int startidx = -1, stopidx = -1;
    for ( int idx=0; idx<depths_->info().getSize(0); idx++ )
    {
	if ( !mIsUdf(depths_->get(idx,colidx)) )
	{
	    startidx = idx;
	    break;
	}

    }

    if ( startidx < 0 )
	return ret;

    for ( int idx=depths_->info().getSize(0)-1; idx>=0; idx-- )
    {
	if ( !mIsUdf(depths_->get(idx,colidx)) )
	{
	    stopidx = idx;
	    break;
	}

    }

    ret.start = origin_.row + startidx * step_.row;
    ret.stop = origin_.row + stopidx * step_.row;
    return ret;
}


StepInterval<int> BinIDSurface::colRange( int row ) const
{
    StepInterval<int> ret( mUdf(int), mUdf(int), step_.col );
    const int rowidx = rowIndex( row );
    if ( rowidx < 0  || !depths_ || rowidx >= depths_->info().getSize(0) )
	return ret;

    int startidy = -1, stopidy = -1;
    for ( int idy=0; idy<depths_->info().getSize(1); idy++ )
    {
	if ( !mIsUdf(depths_->get(rowidx,idy)) )
	{
	    startidy = idy;
	    break;
	}

    }

    if ( startidy < 0 )
	return ret;

    for ( int idy=depths_->info().getSize(1)-1; idy>=0; idy-- )
    {
	if ( !mIsUdf(depths_->get(rowidx,idy)) )
	{
	    stopidy = idy;
	    break;
	}

    }

    ret.start = origin_.col + startidy * step_.col;
    ret.stop = origin_.col + stopidy * step_.col;
    return ret;
}


bool BinIDSurface::expandWithUdf( const BinID& start, const BinID& stop )
{
    if ( !depths_ ) 
	origin_ = RowCol(start);

    const int oldnrrows = nrRows();
    const int oldnrcols = nrCols();

    int startrowidx = rowIndex( start.inl );
    startrowidx = startrowidx>=0 ? 0 : startrowidx;
    int startcolidx = colIndex( start.crl );
    startcolidx = startcolidx>=0 ? 0 : startcolidx;
    int stoprowidx = rowIndex( stop.inl );
    stoprowidx = stoprowidx<oldnrrows ? oldnrrows-1 : stoprowidx;
    int stopcolidx = colIndex( stop.crl );
    stopcolidx = stopcolidx<oldnrcols ? oldnrcols-1 : stopcolidx;
    
    const int newnrrows = stoprowidx-startrowidx+1;
    const int newnrcols = stopcolidx-startcolidx+1;

    if ( oldnrrows==newnrrows && oldnrcols==newnrcols )
	return true;
   
    mDeclareAndTryAlloc( Array2D<float>*, newdepths,
	    		 Array2DImpl<float>( newnrrows, newnrcols ) );
    if ( !newdepths || !newdepths->isOK() )
	return false;

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


Coord BinIDSurface::getKnotCoord( const RowCol& rc) const
{ return SI().transform(BinID(rc)); }


Coord3 BinIDSurface::getKnot( const RowCol& rc, bool interpolifudf ) const
{
    const int index = getKnotIndex( rc );
    const int rowsz = depths_->info().getSize(0);
    const int colsz = depths_->info().getSize(1);
    const int row = index / colsz;
    const int col = index % colsz;
    float posz = index<0 || !depths_ ? mUdf(float) : depths_->get(row,col);
    Coord3 res = Coord3( getKnotCoord( rc ) , posz );

    if ( !depths_ || !mIsUdf(posz) || !interpolifudf )
	return res;
    
    //interpolate
    double diagsum = 0, lateralsum = 0;
    int diagnr = 0, lateralnr = 0;
    for ( int idx=-1; idx<2; idx++ )
    {
	for ( int idy=-1; idy<2; idy++ )
	{
	    if ( !idx && !idy )
		continue;

	    const int currow = row+idx;
	    const int curcol = col+idy;
	    if ( currow<0 || currow>=rowsz || curcol<0 || curcol>=colsz )
		continue;
	    
	    const double curz = depths_->get( currow, curcol );
	    if ( mIsUdf(curz) )
		continue;

	    if ( !idx || !idy )
	    { 
	       lateralsum += curz;
	       lateralnr++;
	    }
	    else
	    {
		diagsum += curz;
		diagnr++;
	    }
	}
    }

    if ( !diagnr && !lateralnr ) //no neighbor defined, do nothing.
	return res;
    
    res.z = (lateralsum+diagsum*0.7071) / (lateralnr + diagnr * 0.7071);

    return res;
}


void BinIDSurface::_setKnot( int idx, const Coord3& np )
{
    if ( !depths_ )
    {
	depths_ = new Array2DImpl<float>( 1, 1 );
	idx = 0;
    }

    const int row = idx / depths_->info().getSize(1);
    const int col = idx % depths_->info().getSize(1);
    depths_->set( row, col, (float)np.z );
}


int BinIDSurface::nrRows() const
{ return depths_ ? depths_->info().getSize(rowDim()) : 0; }


int BinIDSurface::nrCols() const
{ return depths_ ? depths_->info().getSize(colDim()) : 0; }


};

