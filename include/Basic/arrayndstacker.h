#ifndef arrayndstacker_h
#define arrayndstacker_h

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
#include "task.h"

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

    bool	doPrepare( int nrthreads )
		{
		    if ( totalnr_ < 1 )
			return false;

		    for ( int idy=1; idy<inp_.size(); idy++ )
		    {
			if ( !inp_.validIdx(idy) || !inp_[idy] )
			    continue;

			if ( inp_[idy]->info().getTotalSz() != totalnr_ )
			    totalnr_ = inp_[idy]->info().getTotalSz();
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

    bool	doWork( od_int64 start, od_int64 stop, int threadidx )
		{
		    fT* outarr = out_.getData();
		    int count = 0;
		    for ( int idx=start; idx<=stop; idx++ )
		    {
			for ( int idy=0; idy<inp_.size(); idy++ )
			{
			    if ( !inp_[idy] ) continue;
			    const fT* inparr = inp_[idy]->getData();
			    if ( !inparr ||
				 idx >= inp_[idy]->info().getTotalSz() )
			       continue;

			    const fT val = inparr[idx];
			    if ( !mIsUdf(val) )
			    {
				outarr[idx] += val;
				count++;
			    }
			}

			if ( normalize_ && count )
			    outarr[idx] /= count;
		    }

		    return true;
		}

    od_int64	nrIterations() const { return totalnr_; }
    void	doNormalize( bool normalize )	{ normalize_ = normalize; }
    const char*	errMsg() const { return msg_.str(); }

protected:

    od_int64		totalnr_;
    const ObjectSet<ArrT>&	inp_;
    ArrT&		out_;
    bool		normalize_;
    FixedString		msg_;

};


//TODO implement the ND version

#endif

