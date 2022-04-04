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

    od_int64		nrIterations() const override;
    bool		doWork(od_int64 start,od_int64 stop,int) override;
    uiString		uiMessage() const override;

    const Array3D<float>& arr_;
    float		threshold_;
    float		unitvol_;
    float		volsum_;
    Threads::Lock	lock_;
};

