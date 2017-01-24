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
#include "raysynthgenerator.h"
#include "syntheticdata.h"
#include "synthseis.h"
#include "valseriesevent.h"
#include "uistring.h"
#include "stratsynthlevelset.h"

namespace PreStack {  class PreStackSyntheticData; class GatherSetDataPack; }
class RaySynthGenerator;
class SeisTrcBuf;
class TaskRunner;
class Wavelet;
class TrcKeyZSampling;
class RayTracer1D;
class StratSynthLevel;
class PostStackSyntheticData;

namespace Strat
{
    class LayerModel; class LayerModelProvider; class LayerSequence;
    class Level;
}

typedef SyntheticData::RayModel SynthRayModel;
typedef ObjectSet<SynthRayModel> RayModelSet;


mExpClass(WellAttrib) StratSynth
{ mODTextTranslationClass(StratSynth);
public:
    typedef TypeSet<float> LVLZVals;
    typedef TypeSet< LVLZVals > LVLZValsSet;
				StratSynth(const Strat::LayerModelProvider&,
					   bool useed);
				~StratSynth();

    int			nrSynthetics() const;
    RefMan<SyntheticData>	addSynthetic();
    RefMan<SyntheticData>	addSynthetic(const SynthGenParams&);
    bool		removeSynthetic(const char*);
    bool		disableSynthetic(const char*);
    RefMan<SyntheticData>	replaceSynthetic(int id);
    RefMan<SyntheticData>	addDefaultSynthetic();
    int			syntheticIdx(const char* nm) const;
    int			syntheticIdx(const PropertyRef&) const;
    RefMan<SyntheticData>	getSynthetic(const char* nm);
    inline ConstRefMan<SyntheticData> getSynthetic( const char* nm ) const
			{ const int idx = syntheticIdx( nm );
			  return synthetics_.validIdx(idx) ? synthetics_[idx]
							   : 0; }
    void		getSyntheticNames(BufferStringSet&,
					  SynthGenParams::SynthType) const;
    void		getSyntheticNames(BufferStringSet&,bool wantpres) const;
    RefMan<SyntheticData>	getSynthetic(int id);
    RefMan<SyntheticData>	getSynthetic(const PropertyRef&);
    inline ConstRefMan<SyntheticData> getSynthetic(const PropertyRef& prf) const
			{ const int idx = syntheticIdx( prf );
			  return synthetics_.validIdx(idx) ? synthetics_[idx]
							   : 0; }
    RefMan<SyntheticData>	getSyntheticByIdx(int idx);
    ConstRefMan<SyntheticData>	getSyntheticByIdx(int idx) const;
    void		clearSynthetics();
    void		generateOtherQuantities();
    bool		createElasticModels();
    void		clearElasticModels()
					{ aimodels_.erase(); }
    bool		hasElasticModels() const
					{ return !aimodels_.isEmpty(); }

    const ObjectSet<SyntheticData>& synthetics() const
					{ return synthetics_; }

    void		setWavelet(const Wavelet*);
    const Wavelet*	wavelet() const { return wvlt_; }
    SynthGenParams&	genParams()	{ return genparams_; }
    const SynthGenParams& genParams() const
			{ return genparams_; }

    void		setLevels(const StratSynthLevelSet&);
    void		setLevels(const BufferStringSet&,const LVLZValsSet&);
    const StratSynthLevel* getLevel(const int idx) const
				{ return stratlevelset_->getStratLevel(idx); }
    const StratSynthLevelSet* getLevels() const { return stratlevelset_; }

    void		getLevelDepths(const Strat::Level&,
					TypeSet<float>&) const;
    void		getLevelTimes(const Strat::Level&,
				const ObjectSet<const TimeDepthModel>&,
				TypeSet<float>&) const;
    void		getLevelTimes(SeisTrcBuf&,
				const ObjectSet<const TimeDepthModel>&,
				int dispeach=-1) const;
    bool		setLevelTimes(const char* sdnm);

    void		flattenTraces(SeisTrcBuf&) const;
    void		trimTraces(SeisTrcBuf&,
				   const ObjectSet<const TimeDepthModel>&,
				   float zskip) const;
    void		decimateTraces(SeisTrcBuf&,int fac) const;

    void		setTaskRunner(TaskRunner* taskr) { taskr_ = taskr; }
    uiString		errMsg() const;
    uiString		infoMsg() const;
    void		clearInfoMsg()	{ infomsg_.setEmpty(); }

    static const char*	sKeyFRNameSuffix()	{ return " after FR"; }

protected:

    const Strat::LayerModelProvider&	lmp_;
    const bool				useed_;
    StratSynthLevelSet*			stratlevelset_;
    SynthGenParams			genparams_;
    PropertyRefSelection		props_;
    RefObjectSet<SyntheticData>		synthetics_;
    TypeSet<ElasticModel>		aimodels_;
    int					lastsyntheticid_;
    bool				swaveinfomsgshown_;
    const Wavelet*			wvlt_;

    uiString				errmsg_;
    uiString				infomsg_;
    TaskRunner*				taskr_;

    const Strat::LayerModel&	layMod() const;
    bool		canRayModelsBeRemoved(const IOPar& raypar) const;
    bool		fillElasticModel(const Strat::LayerModel&,
					 ElasticModel&,int seqidx);
    bool		adjustElasticModel(const Strat::LayerModel&,
					   TypeSet<ElasticModel>&,bool chksvel);
    void		generateOtherQuantities(
				const PostStackSyntheticData& sd,
				const Strat::LayerModel&);
    RefMan<SyntheticData>	generateSD();
    RefMan<SyntheticData>	generateSD( const SynthGenParams&);
    bool		runSynthGen(RaySynthGenerator&, const SynthGenParams&);
    void		createAngleData(PreStack::PreStackSyntheticData&,
					const ObjectSet<RayTracer1D>&);
    RefMan<SyntheticData>	createAngleStack(const SyntheticData& sd,
					 const TrcKeyZSampling&,
					 const SynthGenParams&);
    RefMan<SyntheticData>	createAVOGradient(const SyntheticData& sd,
					 const TrcKeyZSampling&,
					 const SynthGenParams&);
    RefMan<SyntheticData>	createSynthData(const SyntheticData& sd,
					 const TrcKeyZSampling&,
					 const SynthGenParams&,bool);

    const PreStack::GatherSetDataPack*	getRelevantAngleData(
						const IOPar& raypar) const;

public:
    static uiString	sErrRetMsg() { return uiStrings::phrCannotCreate(tr
				       ("synthetics %1 : %2\n")); }
};
