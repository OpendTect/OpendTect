#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jan 2011
________________________________________________________________________

-*/

#include "algomod.h"

#include "mathfunc.h"
#include "ranges.h"
#include "reflectivitymodel.h"
#include "paralleltask.h"

namespace Fourier { class CC; };

/*!
\brief Takes a ReflectivityModel and samples it in the frequency domain
Applies inverse FFT if a second output is provided
The time sampling determines the frequency distribution
  outsampling_.start should be a multiple of outsampling_.step
*/

mExpClass(Algo) ReflectivitySampler : public ParallelTask
{
public:
			ReflectivitySampler(const ReflectivityModel&,
				const StepInterval<float>& timesampling,
				TypeSet<float_complex>& freqreflectivities,
				bool usenmotime=false);

			~ReflectivitySampler();

				/*<! Available after execution */
    void			getReflectivities(ReflectivityModel&);

protected:
    od_int64			nrIterations() const	{return model_.size();}

    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);
    bool			doFinish(bool);

    void			removeBuffers();
    bool			applyInvFFT(TypeSet<float_complex>&);
    void			getRefModelInterpolator(PointBasedMathFunction&,
							bool valueistime,
							bool isnmo) const;

    const ReflectivityModel&	model_;
    const StepInterval<float>	outsampling_;
    TypeSet<float_complex>&	freqreflectivities_;
    TypeSet<float_complex>*	creflectivities_;
    bool			usenmotime_;

    ObjectSet<float_complex>	buffers_;
};
