#ifndef stratsynth_h
#define stratsynth_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
 RCS:		$Id: stratsynth.h,v 1.1 2011-07-15 12:01:37 cvsbruno Exp $
________________________________________________________________________

-*/

#include "cubesampling.h"
#include "samplingdata.h"
#include "datapack.h"
#include "synthseis.h"

class FlatDataPack;
class TimeDepthModel;
class SeisTrcBuf;
class Wavelet;
namespace Strat { class LayerModel; }


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
    			StratSynth();

    void		setWavelet(const Wavelet&);
    void		setModel(const Strat::LayerModel&);

    const SyntheticData* generate(const RayParams& raypars,bool isps,
				    BufferString* errmsg=0) const;
protected:

    const Strat::LayerModel* lm_;
    const Wavelet*	wvlt_;
    BufferString	errmsg_;

    int			getVelIdx(bool&) const;
    int			getDenIdx(bool&) const;

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
};

#endif
