/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2013
________________________________________________________________________

-*/

#define nullptr 0

#include "matlabarray.h"

#include "arraynd.h"
#include "arrayndinfo.h"
#include "task.h"
#include "varlenarray.h"

ArrayNDCopier::ArrayNDCopier( const ArrayND<float>& arrnd )
    : arrnd_(arrnd)
    , mxarr_(0)
    , totalnr_(-1)
    , managemxarr_(true)
{
}


ArrayNDCopier::~ArrayNDCopier()
{
    if ( managemxarr_ )
	mxDestroyArray( mxarr_ );
}


bool ArrayNDCopier::init( bool managemxarr )
{
    totalnr_ = arrnd_.info().getTotalSz();

    const int nrdim = arrnd_.info().getNDim();
    mAllocVarLenArr( mwSize, dims, nrdim );
    for ( int idx=0; idx<nrdim; idx++ )
	dims[idx] = arrnd_.info().getSize( nrdim-1-idx );

    mxarr_ = mxCreateNumericArray( nrdim, mVarLenArr(dims),
				   mxDOUBLE_CLASS, mxREAL );
    managemxarr_ = managemxarr;

    return true;
}


bool ArrayNDCopier::doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int nrdim = arrnd_.info().getNDim();
    mAllocVarLenArr( int, pos, nrdim );
    double* mxarrptr = mxGetPr( mxarr_ );
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	arrnd_.info().getArrayPos( idx, mVarLenArr(pos) );
	const float fval = arrnd_.getND( mVarLenArr(pos) );
	mxarrptr[idx] = mCast(double,fval);
    }

    return true;
}



// mxArrayCopier
mxArrayCopier::mxArrayCopier( const mxArray& mxarr, ArrayND<float>& arrnd )
    : arrnd_(arrnd)
    , mxarr_(mxarr)
{
}


mxArrayCopier::~mxArrayCopier()
{}


bool mxArrayCopier::init()
{
    totalnr_ = arrnd_.info().getTotalSz();
    const mwSize* dimsz = mxGetDimensions( &mxarr_ );
    const bool samesz = dimsz[0]==arrnd_.info().getSize(2) &&
			dimsz[1]==arrnd_.info().getSize(1) &&
			dimsz[2]==arrnd_.info().getSize(0);
    return samesz;

    // TODO: what if dimensions don't match?
}


bool mxArrayCopier::doWork( od_int64 start, od_int64 stop, int threadid )
{
    double* mxarrptr = mxGetPr( &mxarr_ );
    const int nrdim = arrnd_.info().getNDim();
    mAllocVarLenArr( int, pos, nrdim );
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	arrnd_.info().getArrayPos( idx, mVarLenArr(pos) );
	arrnd_.setND( mVarLenArr(pos), mCast(float,mxarrptr[idx]) );
    }

    return true;
}
