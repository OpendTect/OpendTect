#ifndef stratsynth_h
#define stratsynth_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
 RCS:		$Id: stratsynth.h,v 1.26 2012-09-07 13:24:13 cvshelene Exp $
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "wellattribmod.h"
#include "ailayer.h"
#include "datapack.h"
#include "datapackbase.h"
#include "elasticpropsel.h"
#include "iopar.h"
#include "samplingdata.h"
#include "valseriesevent.h"

class TaskRunner;
class TimeDepthModel;
class PropertyRef;
class PropertyRefSelection;
class SeisTrcBufDataPack;
class SeisTrc;
class SeisTrcBuf;
class Wavelet;
namespace Strat { class LayerModel; class LayerSequence; }
namespace PreStack { class GatherSetDataPack; }


mStruct(WellAttrib) SynthGenParams
{
			SynthGenParams();

    bool		isps_;
    BufferString	name_;
    IOPar		raypars_;
    BufferString	wvltnm_;

    //gen name from wvlt and raypars
    const char*		genName() const;
};



/*! brief the basic synthetic dataset. contains the data cubes*/
mClass(WellAttrib) SyntheticData : public NamedObject 
{
public:
    					~SyntheticData();

    void				setName(const char*);

    virtual const SeisTrc*		getTrace(int seqnr) const = 0;

    float				getTime(float dpt,int seqnr) const;
    float				getDepth(float time,int seqnr) const;

    const DataPack&			getPack() const {return datapack_;}
    DataPack&				getPack() 	{return datapack_;}

    ObjectSet<const TimeDepthModel> 	d2tmodels_;

    DataPack::FullID			datapackid_;

    int					id_;
    virtual bool			isPS() const 		= 0;
    virtual bool			hasOffset() const 	= 0;

    void				useGenParams(const SynthGenParams&);
    void				fillGenParams(SynthGenParams&) const;

protected:
					SyntheticData(const SynthGenParams&,
						      DataPack&);

    BufferString 			wvltnm_;
    IOPar				raypars_;

    void				removePack();

    DataPack&				datapack_;
};


mClass(WellAttrib) PostStackSyntheticData : public SyntheticData
{
public:
				PostStackSyntheticData(const SynthGenParams&,
							SeisTrcBufDataPack&);
				~PostStackSyntheticData();

    bool				isPS() const 	  { return false; }
    bool				hasOffset() const { return false; }

    const SeisTrc*		getTrace(int seqnr) const;

    SeisTrcBufDataPack& 	postStackPack() 
				{ return (SeisTrcBufDataPack&)datapack_; }
    const SeisTrcBufDataPack& 	postStackPack() const
				{ return (SeisTrcBufDataPack&)datapack_; }
};


mClass(WellAttrib) PreStackSyntheticData : public SyntheticData
{
public:
				PreStackSyntheticData(const SynthGenParams&,
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
	{ return (PreStack::GatherSetDataPack&)(datapack_); }
    const PreStack::GatherSetDataPack&	preStackPack() const
	{ return (PreStack::GatherSetDataPack&)(datapack_); }
};


mClass(WellAttrib) PropertyRefSyntheticData : public PostStackSyntheticData
{
public:
				PropertyRefSyntheticData(const SynthGenParams&,
							SeisTrcBufDataPack&,
							const PropertyRef&);

    const PropertyRef&		propRef() const { return prop_; }

protected:
    const PropertyRef& 		prop_;
};


mClass(WellAttrib) StratSynth
{
public:
    				StratSynth(const Strat::LayerModel&);
    				~StratSynth();


    int				nrSynthetics() const; 
    SyntheticData*		addSynthetic(); 
    SyntheticData*		replaceSynthetic(int id);
    SyntheticData*		addDefaultSynthetic(); 
    SyntheticData* 		getSynthetic(const char* nm);
    SyntheticData* 		getSynthetic(int id);
    SyntheticData* 		getSynthetic(const PropertyRef&);
    SyntheticData* 		getSyntheticByIdx(int idx);
    void			clearSynthetics();

    const ObjectSet<SyntheticData>& synthetics() const 	{ return synthetics_; }

    void			setWavelet(const Wavelet*);
    const Wavelet*		wavelet() const { return wvlt_; }
    bool			generate(const Strat::LayerModel&,SeisTrcBuf&);
    SynthGenParams&		genParams()  	{ return genparams_; }


    mStruct(WellAttrib) Level : public NamedObject
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

    void			setTaskRunner(TaskRunner* tr) { tr_ = tr; }
    const char* 		errMsg() const;

protected:

    const Strat::LayerModel& 	lm_;
    const Level*     		level_;
    SynthGenParams		genparams_;
    PropertyRefSelection	props_;
    ObjectSet<SyntheticData> 	synthetics_;
    int				lastsyntheticid_;
    const Wavelet*		wvlt_;

    BufferString		errmsg_;
    TaskRunner*			tr_;

    bool			fillElasticModel(const Strat::LayerModel&,
					ElasticModel&,int seqidx);
    void			generateOtherQuantities( 
	    				PostStackSyntheticData& sd,
	    				const Strat::LayerModel&);
    SyntheticData* 		generateSD(const Strat::LayerModel&,
					TaskRunner* tr=0);

    virtual bool		getVPVSDenValsFromUnitPars(float&,float&,
	    						   float&) const
				{ return false; }

};

#endif


