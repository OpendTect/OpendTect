#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "syntheticdata.h"
#include "ailayer.h"
#include "elasticpropsel.h"
#include "synthseis.h"
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
namespace Seis { class RaySynthGenerator; }

typedef Seis::RaySynthGenerator::RayModel SynthRayModel;
typedef ObjectSet<SynthRayModel> RayModelSet;

mExpClass(WellAttrib) SynthRayModelManager
{ mODTextTranslationClass(SynthRayModelManager);
public:
    ObjectSet<SynthRayModel>*	getRayModelSet(const IOPar&);
    void			addRayModelSet(ObjectSet<SynthRayModel>*,
					       const SyntheticData*);
    void			removeRayModelSet(const IOPar&);
    void			clearRayModels();
    bool			haveSameRM(const IOPar& par1,
					   const IOPar& par2) const;
protected:
    ObjectSet<RayModelSet>	raymodels_;
    TypeSet<IOPar>		synthraypars_;
};



mExpClass(WellAttrib) StratSynth
{ mODTextTranslationClass(StratSynth);
public:
				StratSynth(const Strat::LayerModelProvider&,
					   bool useed);
				~StratSynth();

    int			nrSynthetics() const;
    SyntheticData*	addSynthetic();
    SyntheticData*	addSynthetic(const SynthGenParams&);
    bool		removeSynthetic(const char*);
    bool		disableSynthetic(const char*);
    SyntheticData*	replaceSynthetic(int id);
    SyntheticData*	addDefaultSynthetic();
    int			syntheticIdx(const char* nm) const;
    int			syntheticIdx(const PropertyRef&) const;
    SyntheticData*	getSynthetic(const char* nm);
    inline const SyntheticData* getSynthetic( const char* nm ) const
			{ const int idx = syntheticIdx( nm );
			  return synthetics_.validIdx(idx) ? synthetics_[idx]
							   : 0; }
    void		getSyntheticNames(BufferStringSet&,
					  SynthGenParams::SynthType) const;
    void		getSyntheticNames(BufferStringSet&,bool wantpres) const;
    SyntheticData*	getSynthetic(int id);
    SyntheticData*	getSynthetic(const PropertyRef&);
    inline const SyntheticData* getSynthetic( const PropertyRef& prf ) const
			{ const int idx = syntheticIdx( prf );
			  return synthetics_.validIdx(idx) ? synthetics_[idx]
							   : 0; }
    SyntheticData*	getSyntheticByIdx(int idx);
    const SyntheticData* getSyntheticByIdx(int idx) const;
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

    void		setLevel(const StratSynthLevel*);
    const StratSynthLevel* getLevel() const { return level_; }

    void		getLevelDepths(const Strat::Level&,
					TypeSet<float>&) const;
    void		getLevelTimes(const Strat::Level&,
				const ObjectSet<const TimeDepthModel>&,
				TypeSet<float>&) const;
    void		getLevelTimes(SeisTrcBuf&,
				const ObjectSet<const TimeDepthModel>&) const;
    bool		setLevelTimes(const char* sdnm);

    void		flattenTraces(SeisTrcBuf&) const;
    void		trimTraces(SeisTrcBuf&,
				   const ObjectSet<const TimeDepthModel>&,
				   float zskip) const;
    void		decimateTraces(SeisTrcBuf&,int fac) const;
    void		clearRayModels()
			{ synthrmmgr_.clearRayModels(); }

    void		setTaskRunner( TaskRunner* t )	{ taskr_ = t; }
    bool		hasTaskRunner() const		{ return taskr_; }
    uiString		errMsg() const;
    uiString		infoMsg() const;
    void		clearInfoMsg()	{ infomsg_.setEmpty(); }

    static const char*	sKeyFRNameSuffix()	{ return " after FR"; }
    static void		addFRNameSuffixIfNotAdded(BufferString&);

protected:

    const Strat::LayerModelProvider& lmp_;
    const bool			useed_;
    const StratSynthLevel*	level_;
    SynthGenParams		genparams_;
    PropertyRefSelection	props_;
    ObjectSet<SyntheticData>	synthetics_;
    TypeSet<ElasticModel>	aimodels_;
    int				lastsyntheticid_;
    bool			swaveinfomsgshown_;
    const Wavelet*		wvlt_;

    uiString			errmsg_;
    uiString			infomsg_;
    TaskRunner*			taskr_;
    SynthRayModelManager	synthrmmgr_;

    const Strat::LayerModel&	layMod() const;
    bool		canRayModelsBeRemoved(const IOPar& raypar) const;
    bool		fillElasticModel(const Strat::LayerModel&,
					 ElasticModel&,int seqidx);
    bool		adjustElasticModel(const Strat::LayerModel&,
					   TypeSet<ElasticModel>&,bool chksvel);
    void		generateOtherQuantities(
				const PostStackSyntheticData& sd,
				const Strat::LayerModel&);
    SyntheticData*	generateSD();
    SyntheticData*	generateSD( const SynthGenParams&);
    bool		runSynthGen(Seis::RaySynthGenerator&,
				    const SynthGenParams&);
    SyntheticData*	createAngleStack(const SyntheticData& sd,
					 const TrcKeyZSampling&,
					 const SynthGenParams&);
    SyntheticData*	createAVOGradient(const SyntheticData& sd,
					 const TrcKeyZSampling&,
					 const SynthGenParams&);
    void		createAngleData(PreStackSyntheticData&,
					const ObjectSet<RayTracer1D>&);

    void		adjustD2TModels(ObjectSet<TimeDepthModel>&) const;
    void		putD2TModelsInSD(SyntheticData&,
					 ObjectSet<SynthRayModel>&);

    const PreStack::GatherSetDataPack*	getRelevantAngleData(
						const IOPar& raypar) const;
public:
    void		getLevelTimes(SeisTrcBuf&,
				const ObjectSet<const TimeDepthModel>&,
				int dispeach) const;
};

