#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
________________________________________________________________________

-*/

#include "seismod.h"
#include "syntheticdata.h"

class RayTracer1D;
class ElasticModel;
class SeisTrcBufDataPack;
class PropertyRef;
class SeisTrcBuf;


mExpClass(Seis) PostStackSyntheticData : public SyntheticData
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


mExpClass(Seis) PSBasedPostStackSyntheticData
				: public PostStackSyntheticData
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


mExpClass(Seis) AVOGradSyntheticData
		: public PSBasedPostStackSyntheticData
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


mExpClass(Seis) AngleStackSyntheticData
		: public PSBasedPostStackSyntheticData
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

mExpClass(Seis) StratPropSyntheticData : public PostStackSyntheticData
{
public:
				StratPropSyntheticData(const SynthGenParams&,
							SeisTrcBufDataPack&,
							const PropertyRef&);

    const PropertyRef&		propRef() const { return prop_; }
    SynthGenParams::SynthType	synthType() const
				{ return SynthGenParams::StratProp; }

protected:
    const PropertyRef& 		prop_;
};
