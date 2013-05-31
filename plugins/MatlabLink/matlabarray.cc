/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "matlabarray.h"

#include "arraynd.h"
#include "arrayndinfo.h"
#include "task.h"

#ifdef HAS_MATLAB

ArrayNDCopier::ArrayNDCopier( const ArrayND<float>& arrnd )
    : arrnd_(arrnd)
    , mxarr_(0)
    , totalnr_(-1)
{
}


ArrayNDCopier::~ArrayNDCopier()
{
    mxDestroyArray( mxarr_ );
}


bool ArrayNDCopier::init()
{
    totalnr_ = arrnd_.info().getTotalSz();

    const int nrdim = arrnd_.info().getNDim();
    mwSize dims[nrdim];
    for ( int idx=0; idx<nrdim; idx++ )
	dims[idx] = arrnd_.info().getSize( idx );

    mxarr_ = mxCreateNumericArray( nrdim, dims, mxDOUBLE_CLASS, mxREAL );
    return true;
}


bool ArrayNDCopier::doWork( od_int64 start, od_int64 stop, int threadid )
{
    const int nrdim = arrnd_.info().getNDim();
    int pos[nrdim];
    double* mxarrptr = mxGetPr( mxarr_ );
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	arrnd_.info().getArrayPos( idx, pos );
	mxarrptr[idx] = mCast(double,arrnd_.getND(pos));
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
    const bool samesz = dimsz[0]==arrnd_.info().getSize(0) &&
			dimsz[1]==arrnd_.info().getSize(1) &&
			dimsz[2]==arrnd_.info().getSize(2);
    return samesz;

    // TODO: what if dimensions don't match?
}


bool mxArrayCopier::doWork( od_int64 start, od_int64 stop, int threadid )
{
    // TODO: what if dimensions don't match?
    double* mxarrptr = mxGetPr( &mxarr_ );
    const int nrdim = arrnd_.info().getNDim();
    int pos[nrdim];
    for ( int idx=mCast(int,start); idx<=stop; idx++ )
    {
	arrnd_.info().getArrayPos( idx, pos );
	arrnd_.setND( pos, mCast(float,mxarrptr[idx]) );
    }

    return true;
}

#endif

