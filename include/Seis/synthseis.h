#ifndef synthseis_h
#define synthseis_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: synthseis.h,v 1.24 2011-07-20 13:17:35 cvsbruno Exp $
________________________________________________________________________

-*/

#include "ailayer.h"
#include "cubesampling.h"
#include "factory.h"
#include "odmemory.h"
#include "raytrace1d.h"
#include "samplingdata.h"
#include "task.h"
#include "executor.h"

#include "complex"

class RayTracer1D;
class SeisTrc;
class TimeDepthModel;
class TaskRunner;
class Wavelet;

typedef std::complex<float> float_complex;
namespace Fourier { class CC; };
namespace PreStack { class Gather; }

namespace Seis
{

/* 
   brief generates synthetic traces.The SynthGenerator performs the basic 
   convolution with a reflectivity series and a wavelet.
   The MultiTraceSynthGenerator is a Parallel runner of the SynthGenerator. 

   If you have AI layers and want directly some synthetics out of them, 
   then you should use the RayTraceSynthGenerator. 
*/


mClass SynthGenBase 
{
public:
    virtual bool		setWavelet(const Wavelet*,OD::PtrPolicy pol);
    virtual bool		setOutSampling(const StepInterval<float>&);

    virtual void 		setConvolDomain(bool fourier) 
    				{ isfourier_ = fourier; }

    const char*			errMsg() const	
    				{ return errmsg_.isEmpty() ? 0 : errmsg_.buf();}

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    static const char*		sKeyFourier() 	{ return "Convolution Domain"; }
    static const char* 		sKeyNMO() 	{ return "Use NMO"; }

protected:
    				SynthGenBase();
    				~SynthGenBase();

    bool			isfourier_;
    bool			usenmotimes_;
    bool			waveletismine_;
    const Wavelet*		wavelet_;
    StepInterval<float>		outputsampling_;

    BufferString		errmsg_;
};



mClass SynthGenerator : public SynthGenBase
{
public:
    				SynthGenerator();
    				~SynthGenerator();

    virtual bool		setWavelet(const Wavelet*,OD::PtrPolicy pol);
    virtual bool		setOutSampling(const StepInterval<float>&);
    bool			setModel(const ReflectivityModel&);

    bool                        doPrepare();
    bool			doWork();
    const SeisTrc&		result() const		{ return outtrc_; }

    void 			getSampledReflectivities(TypeSet<float>&) const;

protected:

    bool 			computeTrace(float* result); 
    bool 			doTimeConvolve(float* result); 
    bool 			doFFTConvolve(float* result);
    void 			setConvDomain(bool fourier);

    const ReflectivityModel*	refmodel_;

    Fourier::CC*                fft_;
    int				fftsz_;
    float_complex*		freqwavelet_;
    bool			needprepare_;	
    TypeSet<float_complex>	cresamprefl_;
    SeisTrc&			outtrc_;

    bool			doresample_;
public:
    void			setDoResample(bool yn) 	{ doresample_ = yn; }
};


mClass MultiTraceSynthGenerator : public ParallelTask, public SynthGenBase
{
public:
    				~MultiTraceSynthGenerator();

    void 			setModels(
				    const ObjectSet<const ReflectivityModel>&);

    void 			result(ObjectSet<const SeisTrc>&) const;
    void 			getSampledReflectivities(TypeSet<float>&) const;

    const char*                 message() const 
    					{ return "Generating synthetics..."; }

protected:

    od_int64            	nrIterations() const;
    virtual bool        	doWork(od_int64,od_int64,int);


    ObjectSet<SynthGenerator>	synthgens_;
};



mClass RaySynthGenerator : public SynthGenBase
{
public:
			RaySynthGenerator();
			~RaySynthGenerator();

    //input
    virtual void	addModel(const AIModel&);
    virtual void	setRayParams(const RayTracer1D::Setup&,
				     const TypeSet<float>& offsets,
				     bool isnmo);
    //execute functions
    bool		doWork(TaskRunner* tr=0);
    bool		doRayTracing(TaskRunner* tr=0);
    bool		doSynthetics(TaskRunner* tr=0); 

    mStruct RayModel
    {
			RayModel(const RayTracer1D& rt1d,int nroffsets);
			~RayModel();	

	void 		getTraces(ObjectSet<const SeisTrc>&,bool steal);
	void		getRefs(ObjectSet<const ReflectivityModel>&,bool steal);
	void		getD2T(ObjectSet<const TimeDepthModel>&,bool steal);
	void		getSampledRefs(TypeSet<float>&) const;

	const SeisTrc*	stackedTrc() const;

    protected:
	ObjectSet<const SeisTrc>		outtrcs_; //this is a gather
	ObjectSet<const TimeDepthModel> 	t2dmodels_;
	ObjectSet<const ReflectivityModel> 	refmodels_;
	TypeSet<float>  			sampledrefs_;

    public:
	void		forceReflTimes(const StepInterval<float>&);

	friend class 				RaySynthGenerator;
    };

    //available after execution
    RayModel&		result(int id) 		{ return *raymodels_[id]; }
    const RayModel&	result(int id) const 	{ return *raymodels_[id]; }

    const Interval<float>&	raySampling() const { return raysampling_; }

protected:

    TypeSet<AIModel>		aimodels_;
    RayTracer1D::Setup 		raysetup_;
    TypeSet<float>		offsets_;
    Interval<float>		raysampling_;

    ObjectSet<RayModel>		raymodels_;
};

} //namespace


#endif

