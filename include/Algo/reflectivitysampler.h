#ifndef reflectivitysampler_h
#define reflectivitysampler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jan 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "algomod.h"
#include "ranges.h"
#include "reflectivitymodel.h"
#include "task.h"

namespace Fourier { class CC; };

/*!
\brief Takes a ReflectivityModel and samples it in either frequency or
time domain.
*/

mExpClass(Algo) ReflectivitySampler : public ParallelTask
{
public:
    			ReflectivitySampler(const ReflectivityModel&,
				const StepInterval<float>& timesampling,
				TypeSet<float_complex>& output,
				bool usenmotime=false);

			~ReflectivitySampler();

    void		setTargetDomain(bool fourier);
    			/*!<Default is time-domain */

protected:
    od_int64			nrIterations() const	{return model_.size();}

    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);
    bool			doFinish(bool);

    void			removeBuffers();

    const ReflectivityModel&	model_;
    const StepInterval<float>	outsampling_;
    TypeSet<float_complex>&	output_;
    bool			usenmotime_;
    Fourier::CC*		fft_;

    ObjectSet<float_complex>	buffers_;
};


#endif

