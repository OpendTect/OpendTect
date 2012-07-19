#ifndef stratsynth_h
#define stratsynth_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
 RCS:		$Id: stratsynth.h,v 1.20 2012-07-19 15:12:35 cvsbruno Exp $
________________________________________________________________________

-*/

#include "ailayer.h"
#include "datapack.h"
#include "datapackbase.h"
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
					~SyntheticData();

    void				setName(const char*);

    virtual const SeisTrc*		getTrace(int seqnr) const = 0;

    float				getTime(float dpt,int seqnr) const;
    float				getDepth(float time,int seqnr) const;

    const DataPack&			getPack() const { return datapack_; }
    DataPack&				getPack() 	{ return datapack_; }

    ObjectSet<const TimeDepthModel> 	d2tmodels_;

    DataPack::FullID			datapackid_;

    int					id_;
    virtual bool			isPS() const 		= 0;
    virtual bool			hasOffset() const 	= 0;

    const IOPar&			rayPars() const	{ return raypars_; }

protected:
					SyntheticData(const char* nm, 
					    const IOPar& raypars,DataPack&);

    IOPar				raypars_;
    void				removePack();

    DataPack&				datapack_;
};


mClass PostStackSyntheticData : public SyntheticData
{
public:
					PostStackSyntheticData(const char* nm, 
					    const IOPar& raypars,
					    SeisTrcBufDataPack&);

    bool				isPS() const 	  { return false; }
    bool				hasOffset() const { return false; }

    const SeisTrc*			getTrace(int seqnr) const;

    SeisTrcBufDataPack& 		postStackPack() 
	{ return (SeisTrcBufDataPack&)datapack_; }
    const SeisTrcBufDataPack& 		postStackPack() const
	{ return (SeisTrcBufDataPack&)datapack_; }
};


mClass PreStackSyntheticData : public SyntheticData
{
public:
					PreStackSyntheticData(const char* nm, 
					    const IOPar& raypars,
					    PreStack::GatherSetDataPack&);

    bool				isPS() const 	  { return true; }
    bool				hasOffset() const;
    const Interval<float>		offsetRange() const; 

    const SeisTrc*			getTrace(int seqnr) const
    					{ return getTrace(seqnr,0); }
    const SeisTrc*			getTrace(int seqnr,int* offset) const;
    SeisTrcBuf*				getTrcBuf(float startoffset,
					    const Interval<float>* offrg) const;


    PreStack::GatherSetDataPack&	preStackPack()
	{ return (PreStack::GatherSetDataPack&)datapack_; }
    const PreStack::GatherSetDataPack&	preStackPack() const
	{ return (PreStack::GatherSetDataPack&)datapack_; }
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
    int				lastsyntheticid_;
    ObjectSet<SyntheticData> 	synthetics_;
    IOPar			raypars_;

    BufferString		errmsg_;
    TaskRunner*			tr_;

    SyntheticData* 		generateSD(const Strat::LayerModel&,
					const IOPar* raypar=0,
					TaskRunner* tr=0);
    bool			fillElasticModel(const Strat::LayerModel&,
					    ElasticModel&,int seqidx);
};

#endif
