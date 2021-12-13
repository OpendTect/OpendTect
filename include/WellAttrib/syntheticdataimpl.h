#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		July 2011
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "syntheticdata.h"

class RayTracer1D;
class ElasticModel;
class SeisTrcBufDataPack;
class PropertyRef;

namespace PreStack { class GatherSetDataPack; class Gather; }


mExpClass(WellAttrib) PostStackSyntheticData : public SyntheticData
{
public:
				PostStackSyntheticData(const SynthGenParams&,
							SeisTrcBufDataPack&);
				~PostStackSyntheticData();

    bool			isPS() const override	   { return false; }
    bool			hasOffset() const override { return false; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::ZeroOffset; }

    const SeisTrc*		getTrace(int seqnr) const override;
    int				nrPositions() const override;

    SeisTrcBufDataPack& postStackPack();
    const SeisTrcBufDataPack&	postStackPack() const;

    static const char*		sDataPackCategory();

};


mExpClass(WellAttrib) PostStackSyntheticDataWithInput
				: public PostStackSyntheticData
{
public:
				PostStackSyntheticDataWithInput(
				    const SynthGenParams&,SeisTrcBufDataPack&);
				~PostStackSyntheticDataWithInput();

    void			useGenParams(const SynthGenParams&);
    void			fillGenParams(SynthGenParams&) const;

protected:
    BufferString		inpsynthnm_;
};


mExpClass(WellAttrib) InstAttributeSyntheticData
		: public PostStackSyntheticDataWithInput
{
public:
				InstAttributeSyntheticData(
				    const SynthGenParams& sgp,
				    SeisTrcBufDataPack& sbufdp );

    bool			isAttribute() const override	{ return true; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::InstAttrib; }

    void			useGenParams(const SynthGenParams&);
    void			fillGenParams(SynthGenParams&) const;

protected:
    Attrib::Instantaneous::OutType	attribtype_;
};


mExpClass(WellAttrib) PSBasedPostStackSyntheticData
				: public PostStackSyntheticDataWithInput
{
public:
				PSBasedPostStackSyntheticData(
				    const SynthGenParams&,SeisTrcBufDataPack&);
				~PSBasedPostStackSyntheticData();

    void			useGenParams(const SynthGenParams&);
    void			fillGenParams(SynthGenParams&) const;

protected:
    Interval<float>		anglerg_;
};


mExpClass(WellAttrib) AVOGradSyntheticData
		: public PSBasedPostStackSyntheticData
{
public:
				AVOGradSyntheticData(
				    const SynthGenParams& sgp,
				    SeisTrcBufDataPack& sbufdp )
				    : PSBasedPostStackSyntheticData(sgp,sbufdp)
				{}

    bool			isAVOGradient() const override	{ return true; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::AVOGradient; }
protected:
};


mExpClass(WellAttrib) AngleStackSyntheticData
		: public PSBasedPostStackSyntheticData
{
public:
				AngleStackSyntheticData(
				    const SynthGenParams& sgp,
				    SeisTrcBufDataPack& sbufdp )
				    : PSBasedPostStackSyntheticData(sgp,sbufdp)
				{}

    bool			isAngleStack() const override	{ return true; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::AngleStack; }
protected:
};


mExpClass(WellAttrib) PreStackSyntheticData : public SyntheticData
{
public:
				PreStackSyntheticData(const SynthGenParams&,
						PreStack::GatherSetDataPack&);
				~PreStackSyntheticData();

    bool				isPS() const override  { return true; }
    bool				isNMOCorrected() const;
    bool				hasOffset() const override;
    const Interval<float>		offsetRange() const;
    float				offsetRangeStep() const;
    SynthGenParams::SynthType		synthType() const override
					{ return SynthGenParams::PreStack; }
    int					nrPositions() const override;

    void				setAngleData(
					    const ObjectSet<PreStack::Gather>&);
    const SeisTrc*			getTrace(int seqnr) const
					{ return getTrace(seqnr,0); }
    const SeisTrc*			getTrace(int seqnr,int* offset) const;
    SeisTrcBuf*				getTrcBuf(float startoffset,
					    const Interval<float>* of=0) const;

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

    bool			isStratProp() const override	{ return true; }

    const PropertyRef&		propRef() const { return prop_; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::StratProp; }

protected:
    const PropertyRef&		prop_;
};


