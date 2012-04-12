#ifndef bodyvolumecalc_h
#define bodyvolumecalc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		April 2012
 RCS:		$Id: bodyvolumecalc.h,v 1.1 2012-04-12 16:43:49 cvsyuancheng Exp $
________________________________________________________________________

-*/


#include "arraynd.h"
#include "cubesampling.h"
#include "task.h"

namespace Threads { class Mutex; }

/*!Volume estimate for implicit body in meter. */ 

mClass BodyVolumeCalculator: public ParallelTask
{
public:
    			BodyVolumeCalculator(const CubeSampling& cs,
					     const Array3D<float>& arr,
   					     float threshold,
   					     float velocityinmeter);
float 			getVolume() const	{ return volsum_; }

protected:

od_int64		nrIterations() const	{ return cs_.nrZ()-1; }
bool			doWork(od_int64 start,od_int64 stop,int threadid);

const CubeSampling&	cs_;
const Array3D<float>&	arr_;
float			threshold_;
float			zfactor_;
float			volsum_;
Threads::Mutex		lock_;
};

#endif
