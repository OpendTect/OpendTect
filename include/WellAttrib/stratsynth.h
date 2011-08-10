#ifndef stratsynth_h
#define stratsynth_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
 RCS:		$Id: stratsynth.h,v 1.6 2011-08-10 15:03:51 cvsbruno Exp $
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
class Wavelet;
namespace Strat { class LayerModel; class LayerSequence; }


mStruct RayParams
{				RayParams()
				    : usenmotimes_(true)
				    , dostack_(false)
				    , synthname_("Synthetics")
				{
				    cs_.hrg.setInlRange(Interval<int>(1,1));
				    cs_.hrg.setCrlRange(Interval<int>(0,0));
				    cs_.hrg.step = BinID( 1, 100 );
				    cs_.zrg.set( 0, 0, 0  );
				}
				RayParams(const RayParams& rp)
				{
				    cs_ = rp.cs_;
				    usenmotimes_ = rp.usenmotimes_;
				    dostack_ = rp.dostack_;
				    IOPar spar; rp.setup_.fillPar( spar );
				    setup_.usePar( spar );
				    synthname_ = rp.synthname_;
				}
    CubeSampling 		cs_; //inl are models, crl are offsets
    bool			usenmotimes_;
    bool			dostack_;
    RayTracer1D::Setup		setup_;
    BufferString		synthname_;
};


/*! brief the basic synthetic dataset. Keep traces of how it was buit */
mClass SyntheticData : public NamedObject 
{
public:
				SyntheticData(const char* nm)
				    : NamedObject(nm)
				    , packid_(DataPack::cNoID())
				    {}
				~SyntheticData();

    DataPack::FullID 		packid_;
    ObjectSet<const TimeDepthModel> d2tmodels_;
    bool			isps_;

    RayParams			raypars_;
    const Wavelet*		wvlt_;
};



mClass StratSynth
{
public:
    			StratSynth(const Strat::LayerModel&);

    void		setWavelet(const Wavelet&);

    const SyntheticData* generate(const RayParams& raypars,bool isps,
				    BufferString* errmsg=0) const;
protected:

    const Strat::LayerModel& lm_;
    const Wavelet*	wvlt_;
    TypeSet<AIModel>	aimodels_;

    BufferString	errmsg_;

    DataPack*		genTrcBufDataPack(const RayParams& raypars,
				ObjectSet<const TimeDepthModel>& d2ts,
	    			BufferString* errmsg=0) const;
    DataPack*		genGatherDataPack(const RayParams& raypars,
				ObjectSet<const TimeDepthModel>& d2ts,
	    			BufferString* errmsg=0) const;
    bool		genSeisBufs(const RayParams& raypars,
	    			ObjectSet<const TimeDepthModel>& d2ts,
				ObjectSet<SeisTrcBuf>&,
	    			BufferString* errmsg=0) const;

    bool		fillElasticModel(ElasticModel&,
				    const Strat::LayerSequence&,
				    BufferString* errmsg=0) const;
};

#endif
