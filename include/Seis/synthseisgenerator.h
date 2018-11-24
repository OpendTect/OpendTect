#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		24-3-1996
________________________________________________________________________

-*/

#include "seismod.h"

#include "factory.h"
#include "raytrace1d.h"
#include "uistrings.h"

class SeisTrc;
class SeisTrcBuf;
class TimeDepthModel;
class Wavelet;
template <class T> class Array1D;
template <class T> class ValueSeries;
namespace Fourier { class CC; };

namespace SynthSeis
{

mExpClass(Seis) RayModel : public RefCount::Referenced
{
public:

			RayModel(const RayTracerData&);

    const RayTracerData& rayTracerOutput() const;

    bool		hasZeroOffsetOnly() const;

    const TimeDepthModel& zeroOffsetD2T() const;

    bool		hasSampledReflectivities() const;
    ReflectivityModelSet& reflModels(bool sampled=false);
    const ReflectivityModelSet& reflModels(bool sampled=false) const;

protected:

    virtual		~RayModel();

private:

    ConstRefMan<RayTracerData>	raytracerdata_;
    ReflectivityModelSet sampledreflmodels_;

    friend class Generator;
    friend class MultiTraceGenerator;

public:
			//Do not use casually
    void		forceReflTimes(const StepInterval<float>&);
};


mExpClass(Seis) RayModelSet : public RefObjectSet<RayModel>
			    , public RefCount::Referenced
{
public:
    bool		hasZeroOffsetOnly() const;
    Interval<float>	getTimeRange(bool usenmo=false) const;

private:

    virtual		~RayModelSet()		{}
};


/*!\brief base class for synthetic trace generators. */

mExpClass(Seis) GenBase
{ mODTextTranslationClass(SynthSeis::GenBase);
public:

    virtual void	setWavelet(const Wavelet*);

			/* auto computed + will be overruled if too small */
    void		setOutSampling(const ZSampling&);

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static bool		sStdFFTConvolve()	{ return true; }
    static float	cStdStretchLimit()	{ return 0.2f; }
    static float	cStdMuteLength()	{ return 0.02f; }

    static const char*	sKeyFourier()	{ return "Convolution Domain"; }
    static const char*	sKeyNMO()	{ return "Use NMO"; }
    static const char*  sKeyInternal()  { return "Internal Multiples"; }
    static const char*  sKeySurfRefl()	{ return "Surface Reflection coef"; }
    static const char*	sKeyStretchLimit(){ return "Stretch limit"; }
    static const char*	sKeyMuteLength(){ return "Mute length"; }

protected:

			GenBase();
    virtual		~GenBase();

    virtual uiString	message() const			{ return msg_; }


    bool		setSamplingFromModels(
					const ObjectSet<ReflectivityModel>&);
    void		addWaveletLength(ZSampling&);

    ConstRefMan<Wavelet> wavelet_;
    bool		isfourier_		= sStdFFTConvolve();
    bool		applynmo_		= false;
    bool		dointernalmultiples_	= false;
    float		surfreflcoeff_		= 1.f;
    float		stretchlimit_		= cStdStretchLimit();
    float		mutelength_		= cStdMuteLength();

    ZSampling		outputsampling_;
    ZSampling		worksampling_;
    bool		dosampledreflectivities_	= false;

    mutable uiString	msg_;

    bool		isInputOK();

private:

    static const char*	sKeyWorkRange()		{ return "Convolve ZSampling"; }

};


/*!\brief generates synthetic traces. It performs the basic
   convolution with a reflectivity series and a wavelet.
   The MultiTraceGenerator is a Parallel runner of the Generator.

   If you have AI layers and directly want some synthetics out of them,
   then you should use the RayTraceGenerator.
*/


mExpClass(Seis) Generator : public GenBase
			  , public SequentialTask
{ mODTextTranslationClass(SynthSeis::Generator);
public:

    mDefineFactoryInClass( Generator, factory );

    virtual		~Generator();

    virtual void	setWavelet(const Wavelet*);
    bool		init();

			/*<! can be changed after init */
    bool		setModel(RayModel&,int offsetidx);
    void		setOutput( SeisTrc& trc )	{ trc_ = &trc; }

    bool		isOK() const;

    virtual uiString	message() const final	{ return GenBase::message(); }
    virtual uiString	nrDoneText() const final;

    virtual od_int64	nrDone() const final		{ return nrdone_; }
    virtual od_int64	totalNr() const final;

protected:

			Generator();

    virtual int		nextStep() final;
    bool		setConvolveSize();
    virtual int		computeReflectivities();	//Step 1
    int			computeTrace();			//Step 2

    bool		genFreqWavelet();
    void		getWaveletTrace(Array1D<float>&,float z,float scal,
					SamplingData<float>&) const;
    bool		doFFTConvolve(ValueSeries<float>&,int sz);
    bool		doTimeConvolve(ValueSeries<float>&,int sz);
    bool		doNMOStretch(const ValueSeries<float>&, int insz,
				     ValueSeries<float>& out,int outsz) const;
    void		sortOutput(float_complex*,ValueSeries<float>&,
				   int sz) const;


    RefMan<RayModel>	model_;
    SeisTrc*		trc_		= 0;
    int			offsetidx_	= -1;
    int			convolvesize_	= 0;

    PtrMan<Fourier::CC>		fft_;
    TypeSet<float_complex>	freqwavelet_;
    TypeSet<float_complex>	freqreflectivities_;
    PtrMan<ValueSeries<float> > tmpvals_;

    od_int64		nrdone_;

private:

    static Generator*	create(bool advanced);

    bool		copyInit(const Generator&);

    friend class MultiTraceGenerator;

};


mExpClass(Seis) MultiTraceGenerator : public ParallelTask,
				      public GenBase
{ mODTextTranslationClass(SynthSeis::MultiTraceGenerator);
public:

			MultiTraceGenerator();
    virtual		~MultiTraceGenerator();

    void		set(RayModel&,SeisTrcBuf&,const TrcKey* =0);

    virtual uiString	message() const		{ return GenBase::message(); }
    virtual uiString	nrDoneText() const	{ return Task::sTracesDone(); }

protected:

    virtual od_int64	nrIterations() const final;

private:

    virtual bool	doPrepare(int) final;
    virtual bool	doWork(od_int64,od_int64,int) final;
    virtual bool	doFinish(bool) final;

    RefMan<RayModel>	model_;
    SeisTrcBuf*		trcs_	= 0;
    TrcKey&		tk_;

    ObjectSet<Generator> generators_;
    od_int64		totalnr_;

};

} // namespace SynthSeis
