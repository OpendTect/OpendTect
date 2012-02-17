#ifndef reflectivitysampler_h
#define reflectivitysampler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer
 Date:		Jan 2011
 RCS:		$Id: reflectivitysampler.h,v 1.3 2012-02-17 15:59:45 cvsbruno Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "reflectivitymodel.h"
#include "task.h"

namespace Fourier { class CC; };

/*!Takes a ReflectivityModel and samples it in either frequency or
time domain.
*/


mClass ReflectivitySampler : public ParallelTask
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



mClass InternalMultipleComputer 
{
public:
    			InternalMultipleComputer(const ReflectivityModel& rm,
				    const StepInterval<float>& timesampling,
				    TypeSet<float_complex>& output);

protected:

    mStruct Polynomial : public TypeSet<float_complex> 
    {
				Polynomial() {};
				Polynomial(const Polynomial& P)
				    { *this = P; } 

	    void		substract(const Polynomial& Q);
	    void		multiplyByConst(float_complex cplxval);
	    void		increaseDegree( int by=1 );

	    int 		degree() const { return size(); }
    };

    TypeSet<Polynomial>		Ps_;
    TypeSet<Polynomial>		Qs_;

    const ReflectivityModel&	model_;
    const StepInterval<float>	outsampling_;
    TypeSet<float_complex>&	output_;
    TypeSet<float_complex>	workset_;
    int				size_;

    void			computePolynomials();
    void			computeOutput();
};


#endif
