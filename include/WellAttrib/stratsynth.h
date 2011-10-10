#ifndef stratsynth_h
#define stratsynth_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
 RCS:		$Id: stratsynth.h,v 1.10 2011-10-10 10:14:30 cvsbruno Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "datapack.h"
#include "elasticpropsel.h"
#include "iopar.h"
#include "samplingdata.h"
#include "synthseis.h"

class TimeDepthModel;
class SeisTrcBuf;
class SeisTrcBufDataPack;
class Wavelet;
namespace Strat { class LayerModel; class LayerSequence; }
namespace PreStack { class GatherSetDataPack; }


mStruct RayParams
{				RayParams()
				    : synthname_("Synthetics")
				    , usenmotimes_(true)	
				    , offsetrg_(0,3000,100)
				    {}

				RayParams(const RayParams& rp)
				{
				    usenmotimes_ 	= rp.usenmotimes_;
				    synthname_ 		= rp.synthname_;
				    offsetrg_ 		= rp.offsetrg_;
				    IOPar spar; 
				    rp.setup_.fillPar( spar );
				    setup_.usePar( spar );
				}

    bool 			usenmotimes_;
    BufferString		synthname_;
    RayTracer1D::Setup		setup_;
    StepInterval<float>		offsetrg_;
};


/*! brief the basic synthetic dataset. contains the data cube*/
mClass SyntheticData : public NamedObject 
{
public:
					SyntheticData(const char* nm,
					    PreStack::GatherSetDataPack&);
					~SyntheticData();

    void				setName(const char*);

    const DataPack*			getPack(bool isps) const; 
    ObjectSet<const TimeDepthModel> 	d2tmodels_;

    DataPack::FullID			prestackpackid_;
    DataPack::FullID 			poststackpackid_;

    void				setPostStack(int offset);

    RayParams				raypars_;

protected:
    PreStack::GatherSetDataPack&	prestackpack_;
    SeisTrcBufDataPack*			poststackpack_;

    void				setPack(bool isps,DataPack*);
    void				removePack(bool isps);
};



mClass StratSynth
{
public:
    			StratSynth(const Strat::LayerModel&);
    			~StratSynth();

    const Strat::LayerModel& layerModel() const 	{ return lm_; }

    void		addSynthetics(SyntheticData* sd); 
    SyntheticData* 	getSynthetic( int selid ); 
    const ObjectSet<SyntheticData>& synthetics() const 	{ return synthetics_; }

    void		setWavelet(const Wavelet*);

    RayParams&		rayPars() 	{ return raypars_; }

    const char* 	errMsg() const 
    			{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }

protected:

    RayParams		raypars_;
    const Strat::LayerModel& lm_;
    const Wavelet*	wvlt_;
    TypeSet<AIModel>	aimodels_;

    ObjectSet<SyntheticData> synthetics_;

    BufferString	errmsg_;

    SyntheticData* 	generate();
    bool		fillElasticModel(ElasticModel&,
				    	const Strat::LayerSequence&);
};

#endif
