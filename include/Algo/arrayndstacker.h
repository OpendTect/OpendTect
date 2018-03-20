#pragma once

/*+

________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  Author:        Arnaud Huck
  Date:		 Nov 2013
  RCS:		 $Id:
________________________________________________________________________

-*/

#include "algomod.h"
#include "arraynd.h"
#include "paralleltask.h"

/*!\brief stacks any nymber of compatible ArrayND's into one output.
With normalize_, will return the average instead of the sum
*/

template <class fT>
mClass(Algo) ArrayNDStacker : public ParallelTask
{ mODTextTranslationClass(ArrayNDStacker)
public:

    typedef ArrayND<fT>	ArrT;

ArrayNDStacker( ArrT& out, fT udfval=mUdf(fT) )
    : out_(out)
    , totalnr_(out_.totalSize())
    , normalize_(true)
    , inparrsmine_(false)
    , udfval_(udfval)
{
}

~ArrayNDStacker()
{
    if ( inparrsmine_ )
	deepErase( inps_ );
}

bool addInput( const ArrT* inp )
{
    if ( inp && !inp->isEmpty() )
    {
	if ( inp->nrDims() != out_.nrDims() )
	    { pErrMsg("Dims incompatible"); }
	else
	    { inps_ += inp; return true; }
    }

    if ( inparrsmine_ )
	delete inp;
    return false;
}

ArrayNDStacker& doNormalize( bool yn )
{
    normalize_ = yn;
    return *this;
}

ArrayNDStacker& manageInputs( bool yn )
{
    inparrsmine_ = yn;
    return *this;
}


bool doPrepare( int nrthreads )
{
    if ( inps_.isEmpty() )
	{ out_.setAll( udfval_ ); return true; }

    const ArrayNDInfo::NrDimsType nrdims = out_.nrDims();
    if ( totalnr_ < 1 )
    {
	if ( !out_.canSetInfo() )
	    { pErrMsg("Set sizes first"); }
	else
	{
	    ArrayNDInfoImpl info( nrdims );
	    for ( auto iarr=0; iarr<inps_.size(); iarr++ )
	    {
		const ArrT* inparr = inps_[iarr];
		for ( ArrayNDInfo::DimIdxType idim=0; idim<nrdims; idim++ )
		{
		    if ( info.getSize(idim) < inparr->getSize(idim) )
			info.setSize( idim, inparr->getSize(idim) );
		}
	    }
	    if ( out_.setInfo(info) )
		totalnr_ = out_.totalSize();
	}

	if ( totalnr_ < 1 )
	{
	    msg_ = tr("Cannot set size of stacked data set");
	    return false;
	}
    }

    out_.setAll( udfval_ );
    return true;
}

bool doWork( od_int64 start, od_int64 stop, int threadidx )
{
    if ( totalnr_ < 1 )
	return true;

    mDefNDPosBuf( ndpos, out_.nrDims() );

    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	fT outval = (fT)0;
	out_.info().getArrayPos( idx, ndpos );

	int count = 0;
	for ( auto iarr=0; iarr<inps_.size(); iarr++ )
	{
	    const ArrT* inparr = inps_[iarr];
	    if ( inparr->validPos(ndpos) )
	       continue;

	    const fT val = inparr->getND( ndpos );
	    if ( !mIsUdf(val) )
		{ outval += val; count++; }
	}

	if ( count < 1 )
	    outval = mUdf(fT);
	else if ( normalize_ )
	    outval /= count;

	out_.setND( ndpos, outval );
    }

    return true;
}

od_int64 nrIterations() const
{
    return totalnr_;
}

const uiString errMsg() const
{
    return msg_;
}

protected:

    od_int64		totalnr_;
    ObjectSet<const ArrT> inps_;
    ArrT&		out_;
    fT			udfval_;
    bool		normalize_;
    bool		inparrsmine_;
    uiString		msg_;

};
