#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
________________________________________________________________________

-*/

#include "wellattribmod.h"

#include "ailayer.h"
#include "elasticpropsel.h"
#include "synthseis.h"
#include "syntheticdata.h"
#include "valseriesevent.h"
#include "uistring.h"

namespace PreStack {  class GatherSetDataPack; }
class SeisTrcBuf;
class TaskRunner;
class Wavelet;
class TrcKeyZSampling;
class RayTracer1D;
class StratSynthLevel;
class PostStackSyntheticData;
class PreStackSyntheticData;

namespace Strat
{
    class LayerModel; class LayerModelProvider; class LayerSequence;
    class Level;
}


mExpClass(WellAttrib) StratSynth
{ mODTextTranslationClass(StratSynth);
public:

    mUseType( SyntheticData, SynthID );

				StratSynth(const Strat::LayerModelProvider&,
					   bool useed);
				~StratSynth();

    int			nrSynthetics() const;

    const SyntheticData* addDefaultSynthetic();
    const SyntheticData* addSynthetic(const SynthGenParams&);
    const SyntheticData* replaceSynthetic(SynthID);
    bool		removeSynthetic(const char*);
    bool		disableSynthetic(const char*);
    bool		addInstAttribSynthetics(const BufferStringSet&,
						const SynthGenParams&);

    const SynthFVSpecificDispPars* dispPars(const char* synthnm) const;
    SynthFVSpecificDispPars* dispPars(const char* synthnm);

    const SyntheticData* getSyntheticByIdx(int idx) const;
    const SyntheticData* getSynthetic(const char* nm) const;
    const SyntheticData* getSynthetic(SynthID) const;
    const SyntheticData* getSynthetic(const PropertyRef&) const;

    int			syntheticIdx(const char* nm) const;
    int			syntheticIdx(SynthID) const;
    int			syntheticIdx(const PropertyRef&) const;

    const char*		getSyntheticName(int idx) const;
    void		getSyntheticNames(BufferStringSet&) const;
    void		getSyntheticNames(BufferStringSet&,
					  SynthGenParams::SynthType) const;
    void		getSyntheticNames(BufferStringSet&,bool wantpres) const;
    void		clearSynthetics(bool excludeprops=false);
    void		generateOtherQuantities(double zstep=0.001,
				const BufferStringSet* proplistfilter=nullptr);
			/*!< General all properties, or a subselection
			     Can be overruled by the environment variable
			     DTECT_SYNTHROCK_TIMEPROPS
			  Example: export DTECT_SYNTHROCK_TIMEPROPS=Density|Phie
			  Set to None to disable them all
			 */
    bool		createElasticModels();
    void		clearElasticModels()
					{ aimodels_.erase(); }
    bool		hasElasticModels() const
					{ return !aimodels_.isEmpty(); }

    const ObjectSet<const SyntheticData>& synthetics() const
					{ return synthetics_; }

    void		setWavelet(const Wavelet*);
    const Wavelet*	wavelet() const { return wvlt_; }
    SynthGenParams&	genParams()	{ return genparams_; }
    const SynthGenParams& genParams() const
			{ return genparams_; }

    void		setLevel(const StratSynthLevel*);
    const StratSynthLevel* getLevel() const { return level_; }

    void		getLevelDepths(const Strat::Level&,int each,
				       TypeSet<float>&) const;
    void		getLevelTimes(const Strat::Level&,int each,
				      const ReflectivityModelSet&,
				      TypeSet<float>&,int offsetidx=-1) const;
    void		getLevelTimes(const ReflectivityModelSet&,int each,
				      SeisTrcBuf&,int offsetidx=-1) const;
    bool		setLevelTimes(const char* sdnm,int each,
				      int offsetidx=-1);

    void		flattenTraces(SeisTrcBuf&) const;
    void		trimTraces(SeisTrcBuf&,
				   const ObjectSet<const TimeDepthModel>&,
				   float zskip) const;
    void		decimateTraces(SeisTrcBuf&,int fac) const;

    void		setTaskRunner( TaskRunner* t )	{ taskr_ = t; }
    bool		hasTaskRunner() const		{ return taskr_; }
    uiRetVal		errMsg() const			{ return errmsg_; }
    uiRetVal		infoMsg() const			{ return infomsg_; }
    void		clearInfoMsg()	{ infomsg_.setOK(); }

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    IOPar		getSelection(const BufferStringSet& synthnms) const;

    static bool		getAllGenPars(const IOPar&,ObjectSet<SynthGenParams>&);

    static const char*	sKeyNrSynthetics()	{ return "Nr of Synthetics"; }
    static const char*	sKeySyntheticNr()	{ return "Synthetics Nr"; }
    static const char*	sKeySynthetics()	{ return "Synthetics"; }

    static const char*	sKeyFRNameSuffix()	{ return " after FR"; }

protected:

    const Strat::LayerModelProvider& lmp_;
    const bool			useed_;
    const StratSynthLevel*	level_ = nullptr;
    SynthGenParams		genparams_;
    PropertyRefSelection	props_;
    RefObjectSet<const SyntheticData>	synthetics_;
    TypeSet<ElasticModel>	aimodels_;
    SynthID			lastsyntheticid_ = 0;
    bool			swaveinfomsgshown_ = false;
    const Wavelet*		wvlt_ = nullptr;

    uiRetVal			errmsg_;
    uiRetVal			infomsg_;
    TaskRunner*			taskr_ = nullptr;

    const Strat::LayerModel&	layMod() const;
    bool		fillElasticModel(const Strat::LayerModel&,
					 ElasticModel&,int seqidx);
    bool		adjustElasticModel(const Strat::LayerModel&,
					   TypeSet<ElasticModel>&,bool chksvel);
    bool		runSynthGen(Seis::RaySynthGenerator&,
				    const SynthGenParams&);
    ConstRefMan<SyntheticData> generateSD( const SynthGenParams&);
    ConstRefMan<SyntheticData> createAttribute(const SyntheticData&,
					 const SynthGenParams&);
    ConstRefMan<SyntheticData> createAngleStack(const SyntheticData&,
					 const TrcKeyZSampling&,
					 const SynthGenParams&);
    ConstRefMan<SyntheticData> createAVOGradient(const SyntheticData&,
					 const TrcKeyZSampling&,
					 const SynthGenParams&);
    void		createAngleData(PreStackSyntheticData&);
    void		generateOtherQuantities(const PostStackSyntheticData&,
						const Strat::LayerModel&,
						double zstep,
						const BufferStringSet* nms);

    const ReflectivityModelSet* getRefModels(const SynthGenParams&);
    const Seis::SynthGenDataPack* getSynthGenRes(const SynthGenParams&);
    const PreStack::GatherSetDataPack*	getRelevantAngleData(
					  const Seis::SynthGenDataPack&) const;

};

