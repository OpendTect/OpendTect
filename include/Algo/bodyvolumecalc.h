#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y. Liu
 Date:		April 2012
________________________________________________________________________

-*/


#include "algomod.h"
#include "paralleltask.h"

class TrcKeyZSampling;
template <class T> class Array3D;


/*!
\brief Volume estimation for implicit bodies.
  getVolume returns the volume in cubic meters.
*/

mExpClass(Algo) BodyVolumeCalculator : public ParallelTask
{ mODTextTranslationClass(BodyVolumeCalculator)
public:

			BodyVolumeCalculator(const TrcKeyZSampling&,
					     const Array3D<float>&,
					     float threshold,
					     float velocityinmeter);
    float		getVolume() const	{ return volsum_; }
			//unit in meter^3

protected:

    od_int64		nrIterations() const;
    uiString		nrDoneText() const;
    bool		doWork(od_int64 start,od_int64 stop,int threadid);
    uiString		message() const;

    const Array3D<float>& arr_;
    float		threshold_;
    float		unitvol_;
    float		volsum_;
    float		nrunits_;
    Threads::Lock	lock_;
};
