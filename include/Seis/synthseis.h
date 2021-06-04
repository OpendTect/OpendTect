#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		24-3-1996
________________________________________________________________________

-*/

#include "seismod.h"
#include "ailayer.h"
#include "factory.h"
#include "reflectivitymodel.h"
#include "iopar.h"
#include "odmemory.h"
#include "odcomplex.h"
#include "paralleltask.h"
#include "threadlock.h"
#include "uistrings.h"

class RayTracer1D;
class SeisTrc;
class SeisTrcBuf;
class TimeDepthModel;
class RayTracerRunner;
class Wavelet;
template <class T> class Array1D;
template <class T> class SamplingData;

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


mExpClass(Seis) SynthGenBase
{ mODTextTranslationClass(SynthGenBase);
public:

    virtual bool	setWavelet(const Wavelet*,OD::PtrPolicy pol);
			/* auto computed + will be overruled if too small */
    virtual bool	setOutSampling(const StepInterval<float>&);
			/* depends on the wavelet size too */
    bool		getOutSamplingFromModel
				(const ObjectSet<const ReflectivityModel>&,
				 StepInterval<float>&, bool usenmo=false);

    void		setMuteLength(float n)	{ mutelength_ = n; }
    float		getMuteLength() const	{ return mutelength_; }

    void		setStretchLimit(float n){ stretchlimit_ = n; }
    float		getStretchLimit() const;
    void		doSampledReflectivity(bool yn)
			{ dosampledreflectivities_ = yn; }

    virtual void	enableFourierDomain(bool fourier)
			{ isfourier_ = fourier; }

    uiString		errMsg() const		{ return errmsg_;}

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static float	cStdMuteLength() { return 0.02f; }
    static float	cStdStretchLimit() { return 0.2f; }

    static const char*	sKeyFourier()	{ return "Convolution Domain"; }
    static const char*	sKeyNMO()	{ return "Use NMO"; }
    static const char*  sKeyInternal()  { return "Internal Multiples"; }
    static const char*  sKeySurfRefl()	{ return "Surface Reflection coef"; }
    static const char*	sKeyMuteLength(){ return "Mute length"; }
    static const char*	sKeyStretchLimit(){ return "Stretch limit"; }

protected:
				SynthGenBase();
    virtual			~SynthGenBase();

    bool			isfourier_;
    bool			applynmo_;
    float			stretchlimit_;
    float			mutelength_;
    bool			waveletismine_;
    const Wavelet*		wavelet_;
    StepInterval<float>		outputsampling_;
    bool			dointernalmultiples_;
    bool			dosampledreflectivities_;
    float			surfreflcoeff_;

    uiString			errmsg_;

    bool			isInputOK();
};



mExpClass(Seis) SynthGenerator : public SynthGenBase
{ mODTextTranslationClass(SynthGenerator);
public:
    mDefineFactoryInClass( SynthGenerator, factory );

    static SynthGenerator*	create(bool advanced);

			SynthGenerator();
			~SynthGenerator();

    virtual bool	setWavelet(const Wavelet*,OD::PtrPolicy pol);
			/* auto computed: not necessary -
			   will be overruled if too small */
    virtual bool	setOutSampling(const StepInterval<float>&);
    bool		setModel(const ReflectivityModel&);

    bool		doWork();
    od_int64            currentProgress() const { return progress_; }

    const SeisTrc&	result() const		{ return outtrc_; }
    SeisTrc&		result()		{ return outtrc_; }

			/*<! available after execution */
    const TypeSet<float_complex>& freqReflectivities() const
			{ return freqreflectivities_; }
    void		getSampledRM(ReflectivityModel&) const;


protected:

    int			nextStep();
    int			setConvolveSize();
    int			genFreqWavelet();

    bool		computeTrace(SeisTrc&);
    bool		doNMOStretch(const ValueSeries<float>&, int insz,
				     ValueSeries<float>& out,int outsz) const;
    bool		doFFTConvolve(ValueSeries<float>&,int sz);
    bool		doTimeConvolve(ValueSeries<float>&,int sz);
    void		getWaveletTrace(Array1D<float>&,float z,float scal,
					SamplingData<float>&) const;
    void		sortOutput(float_complex*,ValueSeries<float>&,
				   int sz) const;

    virtual bool	computeReflectivities();

    const ReflectivityModel*	refmodel_;
    int				convolvesize_;
    SeisTrc&			outtrc_;

    ReflectivityModel		sampledrefmodel_;
    TypeSet<float_complex>	freqreflectivities_;
    TypeSet<float_complex>	freqwavelet_;

    od_int64                    progress_;

};


mExpClass(Seis) MultiTraceSynthGenerator : public ParallelTask,
					   public SynthGenBase
{ mODTextTranslationClass(MultiTraceSynthGenerator);
public:
				MultiTraceSynthGenerator();
				~MultiTraceSynthGenerator();

    void			setModels(
				    const ObjectSet<const ReflectivityModel>&);

    void			getResult(ObjectSet<SeisTrc>&);
    void			getSampledRMs(
					ObjectSet<const ReflectivityModel>&);

    uiString			uiMessage() const {
				    return m3Dots(tr("Generating synthetics"));
						  }

    od_int64                    totalNr() const	{ return totalnr_; }

protected:

    od_int64			nrIterations() const;
    bool                        doPrepare(int);
    virtual bool		doWork(od_int64,od_int64,int);

    const ObjectSet<const ReflectivityModel>*	models_;
    ObjectSet<const ReflectivityModel>		sampledrefmodels_;
    ObjectSet<SynthGenerator>	synthgens_;
    ObjectSet<SeisTrc>		trcs_;
    TypeSet<int>		trcidxs_;
    od_int64			totalnr_;
    Threads::Lock		lock_;
};



mExpClass(Seis) RaySynthGenerator : public ParallelTask, public SynthGenBase
{ mODTextTranslationClass(RaySynthGenerator);
public:

    mStruct(Seis) RayModel
    {
			RayModel(const RayTracer1D& rt1d,int nroffsets);
			~RayModel();

	void		getTraces(ObjectSet<SeisTrc>&,bool steal);
	void		getD2T(ObjectSet<TimeDepthModel>&,bool steal);
	void		getZeroOffsetD2T(TimeDepthModel&);
	void		getRefs(ObjectSet<const ReflectivityModel>&,bool steal,
				bool sampled=false);
	void		forceReflTimes(const StepInterval<float>&);

	const SeisTrc*	stackedTrc() const;

    protected:
	ObjectSet<SeisTrc>			outtrcs_; //this is a gather
	ObjectSet<TimeDepthModel>		t2dmodels_;
	TimeDepthModel*				zerooffset2dmodel_;
	ObjectSet<const ReflectivityModel>	refmodels_;
	ObjectSet<const ReflectivityModel>	sampledrefmodels_;

	friend class				RaySynthGenerator;

    };

			RaySynthGenerator(const TypeSet<ElasticModel>*,
					  bool ownrms=true);
			RaySynthGenerator(ObjectSet<RayModel>*);
			~RaySynthGenerator();

    void		reset();

    //input
    void		fillPar(IOPar& raypars) const;
    bool		usePar(const IOPar& raypars);
    void		forceReflTimes(const StepInterval<float>&);

    //available after initialization
    void		getAllRefls(ObjectSet<const ReflectivityModel>&);

    uiString		uiMessage() const
			{ return errmsg_.isEmpty() ? message_ : errmsg_; }


    //available after execution
    RayModel&		result(int id)		{ return *(*raymodels_)[id]; }
    const RayModel&	result(int id) const	{ return *(*raymodels_)[id]; }
    ObjectSet<RayModel>* rayModels()		{ return raymodels_; }

    const ObjectSet<RayTracer1D>& rayTracers() const;
    const TypeSet<ElasticModel>& elasticModels() const	{ return *aimodels_; }
    void		getTraces(ObjectSet<SeisTrcBuf>&);
    void		getStackedTraces(SeisTrcBuf&);

protected:

    RayTracerRunner*		rtr_ = nullptr;
    od_int64			nrIterations() const;
    od_int64			nrDone() const;
    uiString			uiNrDoneText() const;
    od_int64			totalNr() const;
    bool                        doPrepare(int);
    bool			doWork(od_int64,od_int64,int);

    bool			ownraymodels_ = true;
    uiString			message_;
    const TypeSet<ElasticModel>* aimodels_ = nullptr;
    TypeSet<float>		offsets_;
    IOPar			raysetup_;
    ObjectSet<RayModel>*	raymodels_ = nullptr;

    StepInterval<float>		forcedrefltimes_;
    bool			forcerefltimes_ = false;
    bool			raytracingdone_ = false;
};

} // namespace Seis

