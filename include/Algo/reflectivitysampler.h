#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
  timesampling_.start should be a multiple of timesampling_.step
*/

mExpClass(Algo) ReflectivitySampler : public ParallelTask
{
public:

			ReflectivitySampler();
			~ReflectivitySampler();

    void		setInput(const ReflectivityModelTrace&,
			    const float* spikestwt,
			    const ZSampling& timesampling);
    void		setFreqOutput(ReflectivityModelTrace&);
    void		setTimeOutput(ReflectivityModelTrace&,
				  float_complex* tempvals =nullptr,
				  int bufsz=-1);

    bool		unSort(const float_complex*,int inbufsz,
			       float_complex*,int outbufsz) const;
			/*<! Sets the time coefficients back to the same
			     FFT-compatible order */

private:

    od_int64		nrIterations() const override { return totalnr_; }

    bool		doPrepare(int) override;
    bool		doWork(od_int64,od_int64,int) override;
    bool		doFinish(bool) override;

    void		removeBuffers();
    bool		applyInvFFT();
    bool		computeSampledTimeReflectivities();
    void		updateTimeSamplingCache();

    ConstRefMan<ReflectivityModelTrace> model_;
    const float*	spikestwt_;
    ZSampling		outsampling_;
    RefMan<ReflectivityModelTrace> sampledfreqreflectivities_;
    RefMan<ReflectivityModelTrace> sampledtimereflectivities_;

    RefMan<ReflectivityModelTrace> creflectivities_;
    float_complex*		tempreflectivities_ = nullptr;

    ObjectSet<float_complex> buffers_;
    od_int64		totalnr_;

    bool		newsampling_ = false;
    SamplingData<float> fftsampling_;
    float		firsttwt_;
    float		stoptwt_;
    float		width_;
    int			startidx_;
    int			stopidx_;

};
