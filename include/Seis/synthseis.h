#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "factory.h"
#include "odmemory.h"
#include "ranges.h"
#include "reflectivitysampler.h"
#include "seistype.h"

class ElasticModelSet;
class SeisTrc;
class SeisTrcBuf;
class TimeDepthModel;
class Wavelet;
template <class T> class Array1D;
template <class T> class SamplingData;


typedef RefObjectSet<ReflectivityModelTrace> ReflecSet;

namespace Seis
{

class MultiTraceSynthGenerator;
class RaySynthGenerator;

/*
   brief cache for all large data generated during synthetic seismic generation:
	 Time-Depth models, raw and sampled reflectivities in frequency
	 and/or time domain,
	 except for the Seismic Traces
*/

mExpClass(Seis) SynthGenDataPack : public ReferencedObject
{ mODTextTranslationClass(SynthGenDataPack);
public:
				SynthGenDataPack(const ReflectivityModelSet&,
						 GeomType,
						 const TypeSet<float>& offsets,
						 const ZSampling&);

    const ReflectivityModelSet& getModels() const;
    bool			hasSameParams(const SynthGenDataPack&) const;
    bool			hasSameParams(const IOPar& reflpars,
					      const IOPar& synthgenpars) const;
    const ReflecSet*		getSampledReflectivitySet(int imdl,
							  bool freq) const;
    const GeomType&		getType() const		{ return gt_; }
    bool			isStack() const;
    bool			isPS() const;
    int				getOffsetIdx(float offset) const;

private:
				~SynthGenDataPack();
				mOD_DisableCopy(SynthGenDataPack);

    ConstRefMan<ReflectivityModelSet> refmodels_;
    const GeomType		gt_;
    const TypeSet<float>	offsets_;
    const ZSampling		outputsampling_;
    IOPar&			synthgenpars_;
    ObjectSet<const ReflecSet>* freqsampledrefset_ = nullptr;
    ObjectSet<const ReflecSet>* timesampledrefset_ = nullptr;

    friend class RaySynthGenerator;

};

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

    virtual bool	setWavelet(const Wavelet*,OD::PtrPolicy);
    virtual bool	setOutSampling(const ZSampling&);
			//<! auto computed + will be overruled if too small

    void		setMuteLength( float n )	{ mutelength_ = n; }
    float		getMuteLength() const		{ return mutelength_; }

    void		setStretchLimit( float n )	{ stretchlimit_ = n; }
    float		getStretchLimit() const;

    virtual void	enableFourierDomain( bool dofreq )
			{ dofreq_ = dofreq; }
    virtual void	doSampledTimeReflectivity( bool yn )
			{ dosampledtimereflectivities_ = yn; }

    uiString		errMsg() const			{ return errmsg_; }

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

    static bool		cDefIsFrequency();
    static float	cStdMuteLength() { return 0.02f; }
    static float	cStdStretchLimit() { return 0.2f; }

    static const char*	sKeyConvDomain() { return "Convolution Domain"; }
    static const char*	sKeyTimeRefs()	{ return "Time Reflectivities"; }
    static const char*	sKeyNMO()	{ return "Use NMO"; }
    static const char*	sKeyMuteLength(){ return "Mute length"; }
    static const char*	sKeyStretchLimit(){ return "Stretch limit"; }

protected:
				SynthGenBase();
    virtual			~SynthGenBase();

    bool			isInputOK() const;

    bool			dofreq_;
    bool			dosampledtimereflectivities_ = false;
    bool			applynmo_ = false;
    float			stretchlimit_;
    float			mutelength_;
    bool			waveletismine_ = false;
    const Wavelet*		wavelet_ = nullptr;
    ZSampling			outputsampling_;

    mutable uiString		errmsg_;

};



mExpClass(Seis) SynthGenerator : public SynthGenBase
{ mODTextTranslationClass(SynthGenerator);
public:
    mDefineFactoryInClass( SynthGenerator, factory );

			~SynthGenerator();

    static bool		areEquivalent(const IOPar&,const IOPar&);
    virtual bool	isAdvanced() const		{ return false; }

protected:

			SynthGenerator();

    virtual bool	isEquivalent(const SynthGenerator&) const;

    bool		needSampledReflectivities() const;
    virtual bool	outputSampledFreqReflectivities() const;
    virtual bool	outputSampledTimeReflectivities() const;
    virtual bool	needSampledTimeReflectivities() const;

    virtual void	cleanup();
    virtual bool	computeReflectivities();

    const ReflectivityModelTrace* getSampledFreqReflectivities() const;
			/*<! Reflectivities from which the frequency convolution
			     will be done */
    const ReflectivityModelTrace* getSampledTimeReflectivities() const;
			/*<! Time sampled reflectivities from which the
			     time convolution will be done */

    ReflectivityModelTrace* getSampledFreqReflectivities();
    ReflectivityModelTrace* getSampledTimeReflectivities();
    const float_complex* getTempRefs() const	{ return temprefs_; }
    float_complex*	getTempRefs()		{ return temprefs_; }
    const ReflectivitySampler* refSampler() const;
    bool		hasExistingReflectivities() const
						{ return useexistingrefs_; }
    int			getFFTSz() const	{ return convolvesize_; }

    ConstRefMan<ReflectivityModelTrace> refmodel_;
    const float*	spikestwt_ = nullptr;
    const float*	spikescorrectedtwt_ = nullptr;

    SeisTrc*		outtrc_ = nullptr;

private:

    static SynthGenerator* createInstance(const IOPar* =nullptr);

    void		setModel(const ReflectivityModelTrace&,
				 const float* spikestwt_,
				 const float* spikescorrectedtwt_,
				 SeisTrc&);
    void		setSampledFreqReflectivities(ReflectivityModelTrace*);
    void		setSampledTimeReflectivities(ReflectivityModelTrace*);
    void		useSampledFreqReflectivities(
						const ReflectivityModelTrace*);
    void		useSampledTimeReflectivities(
						const ReflectivityModelTrace*);

    bool		setWavelet(const Wavelet*,OD::PtrPolicy) override;
			/* auto computed: not necessary -
			   will be overruled if too small */

    bool		doWork();
    int			nextStep();
    bool		genFreqWavelet();
    bool		computeTrace();

    void		setUnCorrSampling(const ZSampling*);
    int			setConvolveSize();
    bool		doFFTConvolve(ValueSeries<float>&,int sz);
    void		sortOutput(const float_complex*,ValueSeries<float>&,
				   int sz) const;
    bool		doTimeConvolve(ValueSeries<float>&,int sz);
    void		getWaveletTrace(const SamplingData<float>&,
					float z,float scal,
					Array1D<float>&) const;
    bool		doNMOStretch(const ValueSeries<float>&, int insz,
				     ValueSeries<float>& out,int outsz) const;

    PtrMan<ReflectivitySampler> refsampler_;
    RefMan<ReflectivityModelTrace> sampledfreqreflectivities_;
    RefMan<ReflectivityModelTrace> sampledtimereflectivities_;

    RefMan<ReflectivityModelTrace> creflectivities_;
    float_complex*	temprefs_ = nullptr;

    ConstRefMan<ReflectivityModelTrace> csampledfreqreflectivities_;
    ConstRefMan<ReflectivityModelTrace> csampledtimereflectivities_;
    RefMan<ReflectivityModelTrace> freqwavelet_;

    const ZSampling*	uncorrsampling_ = nullptr;
    int			convolvesize_ = 0;
    bool		useexistingrefs_ = false;
    bool		reflectivitiesdone_ = false;

    friend class MultiTraceSynthGenerator;
    friend class RaySynthGenerator;

};


mExpClass(Seis) SynthGeneratorBasic : public SynthGenerator
{ mODTextTranslationClass(SynthGeneratorBasic);
public:

    mDefaultFactoryInstantiation( SynthGenerator, SynthGeneratorBasic,
				  "BasicSynthGenerator",
				  tr("Basic Synthetic Generator") );

};


mClass(Seis) MultiTraceSynthGenerator : public ParallelTask,
					public SynthGenBase
{ mODTextTranslationClass(MultiTraceSynthGenerator);
public:
				~MultiTraceSynthGenerator();

    uiString			uiMessage() const override;

private:
				MultiTraceSynthGenerator(const SynthGenerator&,
					const ReflectivityModelBase&,
					SeisTrcBuf&);

    void			setSampledReflectivitySet(ReflecSet* freqrefset,
							 ReflecSet* timerefset);
    void			useSampledReflectivitySet(
						const ReflecSet* freqrefset,
						const ReflecSet* timerefset);

    od_int64			nrIterations() const override;
    bool			doPrepare(int) override;
    bool			doWork(od_int64,od_int64,int) override;
    bool			doFinish(bool) override;

    const SynthGenerator&	defsynthgen_;
    const ReflectivityModelBase& refmodel_;
    SeisTrcBuf&			trcs_;
    const ReflecSet*		cfreqreflectivityset_ = nullptr;
    const ReflecSet*		ctimereflectivityset_ = nullptr;
    ReflecSet*			freqreflectivityset_ = nullptr;
    ReflecSet*			timereflectivityset_ = nullptr;

    const od_int64		totalnr_;
    uiString			msg_;

    ObjectSet<SynthGenerator>	synthgens_;

    friend class RaySynthGenerator;
};



mExpClass(Seis) RaySynthGenerator : public ParallelTask
{ mODTextTranslationClass(RaySynthGenerator);
public:

    static ConstRefMan<ReflectivityModelSet>
			getRefModels(const ElasticModelSet&,
				     const IOPar& reflpar,uiString& msg,
				     TaskRunner*,float srd,
				     Seis::OffsetType,ZDomain::DepthType,
				     const ObjectSet<const TimeDepthModel>*
						    forcedtdmodels =nullptr);

			RaySynthGenerator(const ReflectivityModelSet&);
			RaySynthGenerator(const SynthGenDataPack&);
			~RaySynthGenerator();

    bool		usePar(const IOPar&);

    bool		setWavelet(const Wavelet*,OD::PtrPolicy);
    bool		setOutSampling(const ZSampling&);
			//<! auto computed + will be overruled if too small
    void		setMuteLength(float);
    void		setStretchLimit(float);
    void		enableFourierDomain(bool yn);
    void		doSampledTimeReflectivity(bool yn);
    void		setTrcStep(int);

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

    //available after execution
    SeisTrc*		stackedTrc(int imdl,bool cansteal=true) const;
    bool		getStackedTraces(SeisTrcBuf&,bool steal=true) const;

    bool		getTraces(SeisTrcBuf&,int imdl,bool steal=true) const;
    bool		getTraces(ObjectSet<SeisTrcBuf>&,bool steal=true) const;
			//<! Each SeisTrcBuf represents a gather

    const SynthGenDataPack* getAllResults();

    static const char*	sKeySynthPar()		{ return "Synth Parameter"; }

private:

    SynthGenerator*		synthgen_;
    ConstRefMan<ReflectivityModelSet> refmodels_;
    TypeSet<float>		offsets_;
    GeomType			rettype_;
    int				trcstep_ = 1;
    mutable uiString		msg_;

    od_int64			nrIterations() const override;

    bool			doPrepare(int) override;
    bool			doWork(od_int64,od_int64,int) override;
    bool			doFinish(bool) override;

    bool			getOutSamplingFromModel(ZSampling& nmozrg,
						    ZSampling* uncorrzrg) const;
    bool			checkPars(bool* skipnmo =nullptr);

    mStruct(Seis) SynthRes
    {
				SynthRes(const ZSampling& outputzrg,
					 const ZSampling& uncorrzrg,
					 const TypeSet<float>& offsets,
					 int seqnr,
					 bool dosampledfreqrefs,
					 bool dosampledtimerefs);
				~SynthRes();

	bool			isOK() const;
	void			setTraces(const ZSampling&,
					  const TypeSet<float>& offsets,
					  int seqnr);

	SeisTrcBuf&		outtrcs_; //this is a gather
	ReflecSet*		freqsampledreflectivityset_ = nullptr;
	ReflecSet*		timesampledreflectivityset_ = nullptr;
    };

    ObjectSet<SynthRes>		results_;
    ConstRefMan<SynthGenDataPack> synthresdp_;

};

} // namespace Seis
