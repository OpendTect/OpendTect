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

/*!
\brief Gently stacks ArrayND by summation along the secondary axis
Currently implemented only 1D, and Array1D's need to return getData() non-null.
With normalize_, will return the average instead of the sum
*/

template <class fT,class ArrT >
mClass(Algo) Array1DStacker : public ParallelTask
{
public:

		Array1DStacker( const ObjectSet<ArrT>& inp,
				ArrT& out )
		    : inp_(inp)
		    , out_(out)
		    , totalnr_(-1)
		    , normalize_(false)
		{
		    if ( !inp.isEmpty() && inp[0] )
			totalnr_ = inp[0]->info().getTotalSz();
		}

    bool	doPrepare( int nrthreads ) override
		{
		    if ( totalnr_ < 1 )
			return false;

		    for ( int iarr=1; iarr<inp_.size(); iarr++ )
		    {
			if ( !inp_[iarr] )
			    continue;

			if ( inp_[iarr]->info().getTotalSz() > totalnr_ )
			    totalnr_ = inp_[iarr]->info().getTotalSz();
		    }

		    out_.setSize( totalnr_ );
		    if ( !out_.getData() )
		    {
			msg_ = "Cannot stack this type of object";
			return false;
		    }

		    out_.setAll( 0 );
		    return true;
		}

    bool	doWork( od_int64 start, od_int64 stop, int threadidx ) override
		{
		    fT* outarr = out_.getData();
		    int count = 0;
		    for ( od_int64 idx=start; idx<=stop; idx++ )
		    {
			fT& outval = outarr[idx];
			for ( int iarr=0; iarr<inp_.size(); iarr++ )
			{
			    if ( !inp_[iarr] ) continue;
			    const fT* inparr = inp_[iarr]->getData();
			    if ( !inparr ||
				 idx >= inp_[iarr]->info().getTotalSz() )
			       continue;

			    const fT val = inparr[idx];
			    if ( !mIsUdf(val) )
			    {
				outval += val;
				count++;
			    }
			}

			if ( normalize_ && count )
			    outval /= count;
		    }

		    return true;
		}

    od_int64	nrIterations() const override { return totalnr_; }
    void	doNormalize( bool normalize )	{ normalize_ = normalize; }
    const char*	errMsg() const { return msg_.str(); }

protected:

    od_int64		totalnr_;
    const ObjectSet<ArrT>&	inp_;
    ArrT&		out_;
    bool		normalize_;
    StringView		msg_;

};


//TODO implement the ND version

