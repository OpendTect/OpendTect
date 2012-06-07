#ifndef synthseis_h
#define synthseis_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: synthseis.h,v 1.32 2012-06-07 08:57:10 cvsbruno Exp $
________________________________________________________________________

-*/

#include "ailayer.h"
#include "factory.h"
#include "reflectivitymodel.h"
#include "iopar.h"
#include "odmemory.h"
#include "task.h"

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

    void			setTaskRunner(TaskRunner* tr) { tr_ = tr; }

    const char*			errMsg() const	
    				{ return errmsg_.isEmpty() ? 0 : errmsg_.buf();}

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    static const char*		sKeyFourier() 	{ return "Convolution Domain"; }
    static const char* 		sKeyNMO() 	{ return "Use NMO"; }
    static const char*  	sKeyInternal()  { return "Internal Multiples"; }
    static const char*  	sKeySurfRefl()  
    					{ return "Surface Reflection coef"; }

protected:
    				SynthGenBase();
    				~SynthGenBase();

    bool			isfourier_;
    bool			usenmotimes_;
    bool			waveletismine_;
    const Wavelet*		wavelet_;
    StepInterval<float>		outputsampling_;
    bool 	                dointernalmultiples_;
    float       	        surfreflcoeff_;

    TaskRunner* 		tr_;

    BufferString		errmsg_;
};



mClass SynthGenerator : public SynthGenBase
{
public:
    mDefineFactoryInClass( SynthGenerator, factory );

    static SynthGenerator*	create(bool advanced);

    				SynthGenerator();
    				~SynthGenerator();

    virtual bool		setWavelet(const Wavelet*,OD::PtrPolicy pol);
    virtual bool		setOutSampling(const StepInterval<float>&);
    bool			setModel(const ReflectivityModel&);

    bool                        doPrepare();
    bool			doWork();
    const SeisTrc&		result() const		{ return outtrc_; }
    SeisTrc&			result() 		{ return outtrc_; }

    void 			getSampledReflectivities(TypeSet<float>&) const;
    virtual void 		setConvolDomain(bool fourier);

protected:

    bool 			computeTrace(float* result); 
    bool 			doTimeConvolve(float* result); 
    bool 			doFFTConvolve(float* result);
    virtual bool		computeReflectivities();

    const ReflectivityModel*	refmodel_;

    Fourier::CC*                fft_;
    int				fftsz_;
    float_complex*		freqwavelet_;
    bool			needprepare_;	
    TypeSet<float_complex>	cresamprefl_;
    SeisTrc&			outtrc_;

    bool			doresample_;

    od_int64                    progress_;


public:
    void			setDoResample(bool yn) 	{ doresample_ = yn; }
    od_int64                    currentProgress() const { return progress_; }
};


mClass MultiTraceSynthGenerator : public ParallelTask, public SynthGenBase
{
public:
    				MultiTraceSynthGenerator();
    				~MultiTraceSynthGenerator();

    void 			setModels(
				    const ObjectSet<const ReflectivityModel>&);

    void 			getResult(ObjectSet<SeisTrc>&); 
    void 			getSampledReflectivities(TypeSet<float>&) const;

    const char*                 message() const 
    					{ return "Generating synthetics..."; }

    od_int64                    totalNr() const         { return totalnr_; }

protected:

    od_int64            	nrIterations() const;
    bool                        doPrepare(int);
    virtual bool        	doWork(od_int64,od_int64,int);

    const ObjectSet<const ReflectivityModel>* models_;
    ObjectSet<SynthGenerator>	synthgens_;
    ObjectSet<SeisTrc>		trcs_;
    TypeSet<int>		trcidxs_;
    od_int64                    totalnr_;
    Threads::Mutex              lock_;
};



mClass RaySynthGenerator : public ParallelTask, public SynthGenBase 
{
public:
			RaySynthGenerator();
			~RaySynthGenerator();

    //input
    void		addModel(const ElasticModel&);
    void		fillPar(IOPar& raypars) const;
    bool		usePar(const IOPar& raypars);

    const char*         message() const { return "Generating synthetics..."; }

    mStruct RayModel
    {
			RayModel(const RayTracer1D& rt1d,int nroffsets);
			~RayModel();	

	void 		getTraces(ObjectSet<SeisTrc>&,bool steal);
	void		getD2T(ObjectSet<TimeDepthModel>&,bool steal);
	void		getRefs(ObjectSet<const ReflectivityModel>&,bool steal);
	void		getSampledRefs(TypeSet<float>&) const;

	const SeisTrc*	stackedTrc() const;

    protected:
	ObjectSet<SeisTrc>			outtrcs_; //this is a gather
	ObjectSet<TimeDepthModel> 		t2dmodels_;
	ObjectSet<const ReflectivityModel> 	refmodels_;
	TypeSet<float>  			sampledrefs_;

	friend class 				RaySynthGenerator;

    public:
	void		forceReflTimes(const StepInterval<float>&);
    };

    //available after execution
    RayModel&		result(int id) 		{ return *raymodels_[id]; }
    const RayModel&	result(int id) const 	{ return *raymodels_[id]; }

    const Interval<float>&	raySampling() const { return raysampling_; }

protected:
    od_int64            	nrIterations() const;
    bool                        doPrepare(int);
    bool        		doWork(od_int64,od_int64,int);

    TypeSet<ElasticModel>	aimodels_;
    TypeSet<float>		offsets_;
    Interval<float>		raysampling_;
    IOPar 			raysetup_;
    ObjectSet<RayModel>		raymodels_;

    StepInterval<float>		forcedrefltimes_;
    bool			forcerefltimes_;

public:
    void			forceReflTimes(const StepInterval<float>&);
};

} //namespace


#endif

