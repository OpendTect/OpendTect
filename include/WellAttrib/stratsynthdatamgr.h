#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "elasticpropsel.h"
#include "raysynthgenerator.h"
#include "syntheticdata.h"
#include "stratsynthlevel.h"
#include "uistring.h"

namespace PreStack {  class PreStackSyntheticData; }
class BufferStringSet;
class GatherSetDataPack;
class RaySynthGenerator;
class SeisTrcBuf;
class TaskRunnerProvider;
class Wavelet;
class TrcKeyZSampling;
class RayTracer1D;
class PostStackSyntheticData;

namespace Strat
{ class LayerModel; class LayerModelProvider; class LayerSequence; }


namespace StratSynth
{

mExpClass(WellAttrib) DataMgr
{ mODTextTranslationClass(StratSynth::DataMgr);
public:

    typedef int					SynthID;
    typedef Level::ID				LevelID;
    typedef Level::ZValueSet			ZValueSet;
    typedef ObjectSet<const TimeDepthModel>	T2DModelSet;

				DataMgr(const Strat::LayerModelProvider&,
					bool useed);
				~DataMgr();

    int			nrSynthetics() const;
    RefMan<SyntheticData> addSynthetic();
    RefMan<SyntheticData> addSynthetic(const SynthGenParams&);
    bool		removeSynthetic(const char*);
    bool		disableSynthetic(const char*);
    RefMan<SyntheticData> replaceSynthetic(int id);
    RefMan<SyntheticData> addDefaultSynthetic();

    int			syntheticIdx(const char* nm) const;
    int			syntheticIdx(const PropertyRef&) const;

    void		getSyntheticNames(BufferStringSet&,
					  SynthGenParams::SynthType) const;
    void		getSyntheticNames(BufferStringSet&,
					  bool wantprestack) const;

    RefMan<SyntheticData>	getSynthetic(SynthID);
    RefMan<SyntheticData>	getSynthetic(const char* nm);
    ConstRefMan<SyntheticData>	getSynthetic(const char* nm) const;
    RefMan<SyntheticData>	getSynthetic(const PropertyRef&);
    ConstRefMan<SyntheticData>	getSynthetic(const PropertyRef&) const;
    RefMan<SyntheticData>	getSyntheticByIdx(int idx);
    ConstRefMan<SyntheticData>	getSyntheticByIdx(int idx) const;

    void		clearSynthetics();
    void		generateOtherQuantities();
    bool		createElasticModels();
    void		clearElasticModels()	{ aimodels_.erase(); }
    bool		hasElasticModels() const
						{ return !aimodels_.isEmpty(); }

    const ObjectSet<SyntheticData>& synthetics() const
						{ return synthetics_; }

    void		setWavelet(const Wavelet*);
    const Wavelet*	wavelet() const		{ return wvlt_; }
    SynthGenParams&	genParams()		{ return genparams_; }
    const SynthGenParams& genParams() const
			{ return genparams_; }

    static void		getTimes(const T2DModelSet&,const ZValueSet& depths,
				    ZValueSet& times);
				//!< 'depths' and 'times' can be the same set

			    // make sure updateLevelInfo() is done before using
			    // one of the functions following it:
    void		updateLevelInfo() const;
    const LevelSet&	levels() const		{ return lvls_; }
    void		getLevelDepths(LevelID,ZValueSet&) const;
    void		getLevelTimes(LevelID,const T2DModelSet&,
				      ZValueSet&) const;
    bool		setLevelTimesInTrcs(LevelID,const char* synthname);
    void		setLevelTimesInTrcs(LevelID,SeisTrcBuf&,
				const T2DModelSet&,int step=-1) const;
    void		flattenTraces(SeisTrcBuf&) const;
    void		trimTraces(SeisTrcBuf&,const T2DModelSet&,
				   float zskip) const;
    void		decimateTraces(SeisTrcBuf&,int fac) const;

    void		setRunnerProvider(const TaskRunnerProvider&);
    uiString		errMsg() const;
    uiString		infoMsg() const;
    void		clearInfoMsg()	{ infomsg_.setEmpty(); }

    static const char*	sKeyFRNameSuffix()	{ return " after FR"; }

protected:

    const Strat::LayerModelProvider&	lmp_;
    const bool				isedited_;
    mutable LevelSet			lvls_;
    SynthGenParams			genparams_;
    PropertyRefSelection		props_;
    RefObjectSet<SyntheticData>		synthetics_;
    TypeSet<ElasticModel>		aimodels_;
    int					lastsyntheticid_;
    bool				swaveinfomsgshown_;
    const Wavelet*			wvlt_;

    uiString				errmsg_;
    uiString				infomsg_;
    const TaskRunnerProvider*		trprov_;

    const Strat::LayerModel&	layMod() const;
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

    const GatherSetDataPack*	getRelevantAngleData(
						const IOPar& raypar) const;

public:

    static uiString	sErrRetMsg() { return uiStrings::phrCannotCreate(tr
				       ("synthetics %1 : %2\n")); }

};

} // namespace StratSynth
