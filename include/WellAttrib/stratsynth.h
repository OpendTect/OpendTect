#ifndef stratsynth_h
#define stratsynth_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
 RCS:		$Id: stratsynth.h,v 1.15 2012-02-08 12:50:16 cvsbruno Exp $
________________________________________________________________________

-*/

#include "ailayer.h"
#include "datapack.h"
#include "elasticpropsel.h"
#include "iopar.h"
#include "samplingdata.h"

class TimeDepthModel;
class SeisTrcBufDataPack;
class SeisTrc;
class SeisTrcBuf;
class Wavelet;
namespace Strat { class LayerModel; class LayerSequence; }
namespace PreStack { class GatherSetDataPack; }


/*! brief the basic synthetic dataset. contains the data cube*/
mClass SyntheticData : public NamedObject 
{
public:
					SyntheticData(const char* nm,
					    PreStack::GatherSetDataPack&);
					~SyntheticData();

    void				setName(const char*);

    const SeisTrc*			getTrace(int seqnr,int* offset=0) const;

    float				getTime(float dpt,int seqnr) const;
    float				getDepth(float time,int seqnr) const;

    const DataPack*			getPack(bool isps) const; 
    const Interval<float>		offsetRange() const; 
    ObjectSet<const TimeDepthModel> 	d2tmodels_;

    DataPack::FullID			prestackpackid_;
    DataPack::FullID 			poststackpackid_;

    void				setPostStack(float offset,
					     const Interval<float>* stackrg=0);
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

    const Wavelet*		wavelet() const		{ return wvlt_; }
    void			setWavelet(const Wavelet*);

    void			clearSynthetics();
    void			addSynthetics(); 
    SyntheticData* 		getSynthetic( int selid );
    const ObjectSet<SyntheticData>& synthetics() const 	{ return synthetics_; }

    IOPar&			rayPars() 		{ return raypars_; }
    const char* 		errMsg() const;

    bool			generate(const Strat::LayerModel&,SeisTrcBuf&);

protected:

    const Strat::LayerModel& 	lm_;
    const Wavelet*		wvlt_;
    BufferString		errmsg_;
    IOPar			raypars_;

    ObjectSet<SyntheticData> 	synthetics_;
    SyntheticData* 		generateSD(const Strat::LayerModel&,
	    					const IOPar* raypar=0);

    bool			fillElasticModel(const Strat::LayerModel&,
					    ElasticModel&,int seqidx);
};

#endif
