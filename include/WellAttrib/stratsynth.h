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
#include "wellattribmod.h"
#include "ailayer.h"
#include "datapack.h"
#include "datapackbase.h"
#include "elasticpropsel.h"
#include "iopar.h"
#include "samplingdata.h"
#include "valseriesevent.h"

class PropertyRef;
class PropertyRefSelection;
class RayTracer1D;
class SeisTrcBufDataPack;
class SeisTrc;
class SeisTrcBuf;
class TaskRunner;
class TimeDepthModel;
class Wavelet;

namespace Strat { class LayerModel; class LayerSequence; }
namespace PreStack { class GatherSetDataPack; class Gather; } 


mStruct(WellAttrib) SynthGenParams
{
			SynthGenParams();

    enum SynthType	{ PreStack, ZeroOffset, AngleStack };
    			DeclareEnumUtils(SynthType);

    SynthType		synthtype_;
    BufferString	name_;
    BufferString	inpsynthnm_;
    IOPar		raypars_;
    BufferString	wvltnm_;
    
    bool		hasOffsets() const;
    bool		isPreStack() const 	{ return synthtype_==PreStack; }
    void		createName(BufferString&) const;
    			//!<Create name from wvlt and raypars
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

bool operator==( const SynthGenParams& gp )
{ return isPreStack()==gp.isPreStack() && wvltnm_==gp.wvltnm_ &&
         raypars_==gp.raypars_; }

};



/*! brief the basic synthetic dataset. contains the data cubes*/
mExpClass(WellAttrib) SyntheticData : public NamedObject 
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
    bool				isAngleStack() const;
    virtual SynthGenParams::SynthType	synthType() const	= 0;

    virtual void			useGenParams(const SynthGenParams&);
    virtual void			fillGenParams(SynthGenParams&) const;
    const char*				waveletName() const { return wvltnm_; }
    void				setWavelet( const char* wvltnm )
					{ wvltnm_ = wvltnm; }

protected:
					SyntheticData(const SynthGenParams&,
						      DataPack&);

    BufferString 			wvltnm_;
    IOPar				raypars_;

    void				removePack();

    DataPack&				datapack_;
};


mExpClass(WellAttrib) PostStackSyntheticData : public SyntheticData
{
public:
				PostStackSyntheticData(const SynthGenParams&,
							SeisTrcBufDataPack&);
				~PostStackSyntheticData();

    bool			isPS() const 	  { return false; }
    bool			hasOffset() const { return false; }
    bool			isAngleStack() const { return false; }
    SynthGenParams::SynthType	synthType() const
				{ return SynthGenParams::ZeroOffset; }

    const SeisTrc*		getTrace(int seqnr) const;

    SeisTrcBufDataPack& 	postStackPack() 
				{ return (SeisTrcBufDataPack&)datapack_; }
    const SeisTrcBufDataPack& 	postStackPack() const
				{ return (SeisTrcBufDataPack&)datapack_; }
};


mExpClass(WellAttrib) AngleStackSyntheticData : public PostStackSyntheticData
{
public:
				AngleStackSyntheticData(const SynthGenParams&,
						SeisTrcBufDataPack&);
				~AngleStackSyntheticData();
    bool			isPS() const 	  { return false; }
    bool			isAngleStack() const { return true; }
    void			useGenParams(const SynthGenParams&);
    void			fillGenParams(SynthGenParams&) const;
    SynthGenParams::SynthType	synthType() const
				{ return SynthGenParams::AngleStack; }
protected:
    BufferString 		inpsynthnm_;
};


mExpClass(WellAttrib) PreStackSyntheticData : public SyntheticData
{
public:
				PreStackSyntheticData(const SynthGenParams&,
						PreStack::GatherSetDataPack&);
				~PreStackSyntheticData();

    bool				isPS() const 	  { return true; }
    bool				hasOffset() const;
    const Interval<float>		offsetRange() const; 
    SynthGenParams::SynthType		synthType() const
					{ return SynthGenParams::PreStack; }

    void				createAngleData(
						const ObjectSet<RayTracer1D>&,
						const TypeSet<ElasticModel>&);
    const SeisTrc*			getTrace(int seqnr) const
    					{ return getTrace(seqnr,0); }
    const SeisTrc*			getTrace(int seqnr,int* offset) const;
    SeisTrcBuf*				getTrcBuf(float startoffset,
					    const Interval<float>* off=0) const;

    PreStack::GatherSetDataPack&	preStackPack()
	{ return (PreStack::GatherSetDataPack&)(datapack_); }
    const PreStack::GatherSetDataPack&	preStackPack() const
	{ return (PreStack::GatherSetDataPack&)(datapack_); }
    const PreStack::GatherSetDataPack&	angleData() const { return *angledp_; }
protected:
    PreStack::GatherSetDataPack*	angledp_;
    void				convertAngleDataToDegrees(
	    					PreStack::Gather*) const;
};


mExpClass(WellAttrib) PropertyRefSyntheticData : public PostStackSyntheticData
{
public:
				PropertyRefSyntheticData(const SynthGenParams&,
							SeisTrcBufDataPack&,
							const PropertyRef&);

    const PropertyRef&		propRef() const { return prop_; }

protected:
    const PropertyRef& 		prop_;
};


mExpClass(WellAttrib) StratSynth
{
public:
    				StratSynth(const Strat::LayerModel&);
    				~StratSynth();


    int				nrSynthetics() const; 
    SyntheticData*		addSynthetic(); 
    SyntheticData*		addSynthetic(const SynthGenParams&); 
    bool			removeSynthetic(const char*);
    SyntheticData*		replaceSynthetic(int id);
    SyntheticData*		addDefaultSynthetic(); 
    SyntheticData* 		getSynthetic(const char* nm);
    SyntheticData* 		getSynthetic(int id);
    SyntheticData* 		getSynthetic(const PropertyRef&);
    SyntheticData* 		getSyntheticByIdx(int idx);
    void			clearSynthetics();
    void			generateOtherQuantities();
    static float		cMaximumVpWaterVel();

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

    void			flattenTraces(SeisTrcBuf&) const;
    void			decimateTraces(SeisTrcBuf&,int fac) const;

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
	    				const PostStackSyntheticData& sd,
	    				const Strat::LayerModel&);
    SyntheticData* 		generateSD(const Strat::LayerModel&,
					TaskRunner* tr=0);
    SyntheticData* 		generateSD(const Strat::LayerModel&,
	    				const SynthGenParams&,
					TaskRunner* tr=0);
    SyntheticData*		createAngleStack(SyntheticData* sd,
	    					 const CubeSampling&,
						 const SynthGenParams&,
						 TaskRunner*);
};

#endif


