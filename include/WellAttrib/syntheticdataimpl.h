#ifndef syntheticdataimpl_h
#define syntheticdataimpl_h

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

class RayTracer1D;
class ElasticModel;
class SeisTrcBufDataPack;

namespace PreStack { class GatherSetDataPack; class Gather; } 


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

    SeisTrcBufDataPack& 	postStackPack();
    const SeisTrcBufDataPack& 	postStackPack() const;

};


mExpClass(WellAttrib) PSBasedPostStackSyntheticData : public PostStackSyntheticData
{
public:
				PSBasedPostStackSyntheticData(
				    const SynthGenParams&,SeisTrcBufDataPack&);
				~PSBasedPostStackSyntheticData();
    void			useGenParams(const SynthGenParams&);
    void			fillGenParams(SynthGenParams&) const;
protected:
    BufferString 		inpsynthnm_;
    Interval<float>		anglerg_;
};


mExpClass(WellAttrib) AVOGradSyntheticData : public PSBasedPostStackSyntheticData
{
public:
				AVOGradSyntheticData(
				    const SynthGenParams& sgp,
				    SeisTrcBufDataPack& sbufdp )
				    : PSBasedPostStackSyntheticData(sgp,sbufdp)
				{}
    bool			isAVOGradient() const 	{ return true; }
    bool			isAngleStack() const 	{ return false; }
    SynthGenParams::SynthType	synthType() const
				{ return SynthGenParams::AVOGradient; }
protected:
};


mExpClass(WellAttrib) AngleStackSyntheticData : public PSBasedPostStackSyntheticData
{
public:
				AngleStackSyntheticData(
				    const SynthGenParams& sgp,
				    SeisTrcBufDataPack& sbufdp )
				    : PSBasedPostStackSyntheticData(sgp,sbufdp)
				{}
    bool			isAVOGradient() const 	{ return false; }
    bool			isAngleStack() const 	{ return true; }
    SynthGenParams::SynthType	synthType() const
				{ return SynthGenParams::AngleStack; }
protected:
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

    PreStack::GatherSetDataPack&	preStackPack();
    const PreStack::GatherSetDataPack&	preStackPack() const;
    const PreStack::GatherSetDataPack&	angleData() const { return *angledp_; }

protected:

    PreStack::GatherSetDataPack*	angledp_;
    void				convertAngleDataToDegrees(
	    					PreStack::Gather*) const;
};


mExpClass(WellAttrib) StratPropSyntheticData : public PostStackSyntheticData
{
public:
				StratPropSyntheticData(const SynthGenParams&,
							SeisTrcBufDataPack&,
							const PropertyRef&);

    const PropertyRef&		propRef() const { return prop_; }

protected:
    const PropertyRef& 		prop_;
};


#endif
