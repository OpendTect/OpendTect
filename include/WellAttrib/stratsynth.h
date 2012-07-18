#ifndef stratsynth_h
#define stratsynth_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
 RCS:		$Id: stratsynth.h,v 1.19 2012-07-18 15:00:36 cvsbruno Exp $
________________________________________________________________________

-*/

#include "ailayer.h"
#include "datapack.h"
#include "elasticpropsel.h"
#include "iopar.h"
#include "samplingdata.h"
#include "valseriesevent.h"

class TaskRunner;
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
					    PreStack::GatherSetDataPack&,
					    const IOPar& raypars);
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

    int					id_;
    const IOPar&			rayPars() const	{ return raypars_; }
    bool				hasOffset() const;

protected:
    PreStack::GatherSetDataPack&	prestackpack_;
    SeisTrcBufDataPack*			poststackpack_;
    IOPar				raypars_;

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

    void			setTaskRunner(TaskRunner* tr) { tr_ = tr; }

    IOPar&			rayPars() 		{ return raypars_; }

    const char* 		errMsg() const;

    int				nrSynthetics() const; 
    SyntheticData*		addSynthetic(const char* nm); 
    SyntheticData*		replaceSynthetic(int id);
    SyntheticData*		addDefaultSynthetic(); 
    SyntheticData* 		getSynthetic(const char* nm);
    SyntheticData* 		getSynthetic(int id);
    SyntheticData* 		getSyntheticByIdx(int idx);
    void			clearSynthetics();

    const ObjectSet<SyntheticData>& synthetics() const 	{ return synthetics_; }

    mStruct Level : public NamedObject
    {
				Level(const char* nm,const TypeSet<float>& dpts,
				    const Color& c)
				: NamedObject(nm), col_(c), zvals_(dpts)  {}

	TypeSet<float>  	zvals_;
	Color           	col_;
	VSEvent::Type   	snapev_;
    };

    void			setLevel(const Level* lvl);
    const Level*		getLevel() const 	{ return level_; }
    void			snapLevelTimes(SeisTrcBuf&,
				    const ObjectSet<const TimeDepthModel>&);

    bool			generate(const Strat::LayerModel&,SeisTrcBuf&);

    const char*			getDefaultSyntheticName() const;

protected:

    const Strat::LayerModel& 	lm_;
    const Wavelet*		wvlt_;
    const Level*     		level_;

    BufferString		errmsg_;
    int				lastsyntheticid_;
    IOPar			raypars_;
    TaskRunner*			tr_;

    ObjectSet<SyntheticData> 	synthetics_;
    SyntheticData* 		generateSD(const Strat::LayerModel&,
					const IOPar* raypar=0,
					TaskRunner* tr=0);
    bool			fillElasticModel(const Strat::LayerModel&,
					    ElasticModel&,int seqidx);
};

#endif
