#ifndef stratsynth_h
#define stratsynth_h

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
#include "valseriesevent.h"

class SeisTrcBuf;
class TaskRunner;
class Wavelet;
class CubeSampling;
class StratSynthLevel;
class PostStackSyntheticData;

namespace Strat
{
    class LayerModel; class LayerModelProvider; class LayerSequence;
    class Level;
}
namespace Seis { class RaySynthGenerator; }


mExpClass(WellAttrib) StratSynth
{
public:
    				StratSynth(const Strat::LayerModelProvider&,
					   bool useed);
    				~StratSynth();

    int			nrSynthetics() const; 
    SyntheticData*	addSynthetic(); 
    SyntheticData*	addSynthetic(const SynthGenParams&); 
    bool		removeSynthetic(const char*);
    SyntheticData*	replaceSynthetic(int id);
    SyntheticData*	addDefaultSynthetic(); 
    int			syntheticIdx(const char* nm) const;
    int			syntheticIdx(const PropertyRef&) const;
    SyntheticData* 	getSynthetic(const char* nm);
    inline const SyntheticData* getSynthetic( const char* nm ) const
			{ const int idx = syntheticIdx( nm );
			  return synthetics_.validIdx(idx) ? synthetics_[idx]
			      				   : 0; }
    SyntheticData* 	getSynthetic(int id);
    SyntheticData* 	getSynthetic(const PropertyRef&);
    inline const SyntheticData* getSynthetic( const PropertyRef& prf ) const
			{ const int idx = syntheticIdx( prf );
			  return synthetics_.validIdx(idx) ? synthetics_[idx]
			      				   : 0; }
    SyntheticData* 	getSyntheticByIdx(int idx);
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
    SynthGenParams&	genParams()  	{ return genparams_; }

    void		setLevel(const StratSynthLevel*);
    const StratSynthLevel* getLevel() const { return level_; }

    void		getLevelDepths(const Strat::Level&,
	    				TypeSet<float>&) const;
    void		getLevelTimes(const Strat::Level&,
				const ObjectSet<const TimeDepthModel>&,
				TypeSet<float>&) const;
    void		getLevelTimes(SeisTrcBuf&,
				const ObjectSet<const TimeDepthModel>&) const;

    void		flattenTraces(SeisTrcBuf&) const;
    void		trimTraces(SeisTrcBuf&,
				   const ObjectSet<const TimeDepthModel>&,
				   float zskip) const;
    void		decimateTraces(SeisTrcBuf&,int fac) const;

    void		setTaskRunner(TaskRunner* tr) { tr_ = tr; }
    const char* 	errMsg() const;
    const char* 	infoMsg() const;
    void		clearInfoMsg()	{ infomsg_.setEmpty(); }

protected:

    const Strat::LayerModelProvider& lmp_;
    const bool			useed_;
    const StratSynthLevel* 	level_;
    SynthGenParams		genparams_;
    PropertyRefSelection	props_;
    ObjectSet<SyntheticData> 	synthetics_;
    TypeSet<ElasticModel>	aimodels_;
    int				lastsyntheticid_;
    const Wavelet*		wvlt_;

    BufferString		errmsg_;
    BufferString		infomsg_;
    TaskRunner*			tr_;

    const Strat::LayerModel&	layMod() const;
    bool		fillElasticModel(const Strat::LayerModel&,
					 ElasticModel&,int seqidx);
    bool		adjustElasticModel(const Strat::LayerModel&,
					   TypeSet<ElasticModel>&);
    void		generateOtherQuantities( 
	    			const PostStackSyntheticData& sd,
	    			const Strat::LayerModel&);
    SyntheticData* 	generateSD();
    SyntheticData* 	generateSD( const SynthGenParams&);
    SyntheticData*	createAngleStack(SyntheticData* sd,
	    				 const CubeSampling&,
					 const SynthGenParams&);
    SyntheticData*	createAVOGradient(SyntheticData* sd,
	    				 const CubeSampling&,
					 const SynthGenParams&,
					 const Seis::RaySynthGenerator&);
};

#endif


