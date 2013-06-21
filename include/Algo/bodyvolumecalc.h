#ifndef bodyvolumecalc_h
#define bodyvolumecalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		April 2012
 RCS:		$Id$
________________________________________________________________________

-*/


#include "algomod.h"
#include "task.h"
#include "threadlock.h"

class CubeSampling;
template <class T> class Array3D;


/*!
\brief Volume estimate for implicit body in meter.
*/

mExpClass(Algo) BodyVolumeCalculator: public ParallelTask
{
public:
    			BodyVolumeCalculator(const CubeSampling& cs,
					     const Array3D<float>& arr,
   					     float threshold,
   					     float velocityinmeter);
    float		getVolume() const	{ return volsum_; }
			//unit in meter^3

protected:

    od_int64		nrIterations() const;
    bool		doWork(od_int64 start,od_int64 stop,int threadid);

    const CubeSampling&	cs_;
    const Array3D<float>& arr_;
    float		threshold_;
    float		unitvol_;
    float		volsum_;
    Threads::Lock	lock_;
};


#endif

