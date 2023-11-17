#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellattribmod.h"

#include "prestackgather.h"
#include "syntheticdata.h"

class SeisTrcBufDataPack;
class PropertyRef;


mExpClass(WellAttrib) PostStackSyntheticData : public SyntheticData
{
public:
				PostStackSyntheticData(const SynthGenParams&,
					       const Seis::SynthGenDataPack&,
					       SeisTrcBufDataPack&);

    DataPack::FullID		fullID() const override;
    bool			isPS() const override	   { return false; }
    bool			hasOffset() const override { return false; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::ZeroOffset; }

    const SeisTrc*		getTrace(int trcnr) const override;
    int				nrPositions() const override;
    TrcKey			getTrcKey(int trcnr) const override;
    ZSampling			zRange() const override;

    const SeisTrcBufDataPack&	postStackPack() const;
    SeisTrcBufDataPack&		postStackPack();

    ConstRefMan<FlatDataPack>	getTrcDP() const override;
    ConstRefMan<FlatDataPack>	getFlattenedTrcDP(const TypeSet<float>& zvals,
						  bool istime) const override;

    static const char*		sDataPackCategory();

protected:
				~PostStackSyntheticData();
private:

    static DataPack::MgrID	groupID();

};


mExpClass(WellAttrib) PostStackSyntheticDataWithInput
				: public PostStackSyntheticData
{
public:
				PostStackSyntheticDataWithInput(
						const SynthGenParams&,
						const Seis::SynthGenDataPack&,
						SeisTrcBufDataPack&);

    void			useGenParams(const SynthGenParams&);
    void			fillGenParams(SynthGenParams&) const;

protected:
				~PostStackSyntheticDataWithInput();

    BufferString		inpsynthnm_;
};


mExpClass(WellAttrib) InstAttributeSyntheticData
		: public PostStackSyntheticDataWithInput
{
public:
				InstAttributeSyntheticData(
						const SynthGenParams&,
						const Seis::SynthGenDataPack&,
						SeisTrcBufDataPack&);

    bool			isAttribute() const override	{ return true; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::InstAttrib; }

    void			useGenParams(const SynthGenParams&);
    void			fillGenParams(SynthGenParams&) const;

private:
				~InstAttributeSyntheticData();

    Attrib::Instantaneous::OutType	attribtype_;
};


mExpClass(WellAttrib) FilteredSyntheticData
		: public PostStackSyntheticDataWithInput
{
public:
				FilteredSyntheticData(
						const SynthGenParams&,
						const Seis::SynthGenDataPack&,
						SeisTrcBufDataPack&);

    SynthGenParams::SynthType	synthType() const override
				{ return sgp_.synthtype_; }

    void			useGenParams(const SynthGenParams&);
    void			fillGenParams(SynthGenParams&) const;

private:
				~FilteredSyntheticData();

    BufferString		filtertype_;
    int				windowsz_;
    Interval<float>		freqrg_;
};


mExpClass(WellAttrib) PSBasedPostStackSyntheticData
				: public PostStackSyntheticDataWithInput
{
public:
				PSBasedPostStackSyntheticData(
						const SynthGenParams&,
						const Seis::SynthGenDataPack&,
						SeisTrcBufDataPack&);

    void			useGenParams(const SynthGenParams&);
    void			fillGenParams(SynthGenParams&) const;

protected:
				~PSBasedPostStackSyntheticData();

    Interval<float>		anglerg_;
};


mExpClass(WellAttrib) AVOGradSyntheticData
				: public PSBasedPostStackSyntheticData
{
public:
				AVOGradSyntheticData(
						const SynthGenParams&,
						const Seis::SynthGenDataPack&,
						SeisTrcBufDataPack&);

    bool			isAVOGradient() const override	{ return true; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::AVOGradient; }

private:
				~AVOGradSyntheticData();
};


mExpClass(WellAttrib) AngleStackSyntheticData
				: public PSBasedPostStackSyntheticData
{
public:
				AngleStackSyntheticData(
						const SynthGenParams&,
						const Seis::SynthGenDataPack&,
						SeisTrcBufDataPack&);

    bool			isAngleStack() const override	{ return true; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::AngleStack; }

private:
				~AngleStackSyntheticData();
};


mExpClass(WellAttrib) PreStackSyntheticData : public SyntheticData
{
public:
				PreStackSyntheticData(const SynthGenParams&,
						const Seis::SynthGenDataPack&,
						PreStack::GatherSetDataPack&);

    DataPack::FullID			fullID() const override;

    void				setName(const char*) override;
    bool				isPS() const override  { return true; }
    bool				isNMOCorrected() const;
    bool				hasOffset() const override;
    Interval<float>			offsetRange() const;
    float				offsetRangeStep() const;
    SynthGenParams::SynthType		synthType() const override
					{ return SynthGenParams::PreStack; }
    int					nrPositions() const override;
    TrcKey				getTrcKey(int trcnr) const override;
    DataPackID			getGatherIDByIdx(int trcnr,
						      bool angles=false) const;
    ZSampling				zRange() const override;

    void				setAngleData(
					    const PreStack::GatherSetDataPack*);
    const SeisTrc*			getTrace(int trcnr) const override
					{ return getTrace(trcnr,nullptr); }
    const SeisTrc*			getTrace(int trcnr,int* offset) const;
    SeisTrcBuf*				getTrcBuf(float startoffset,
						  const Interval<float>* of
								=nullptr) const;

    PreStack::GatherSetDataPack&	preStackPack();
    const PreStack::GatherSetDataPack&	preStackPack() const;
    const PreStack::GatherSetDataPack&	angleData() const { return *angledp_; }
    bool			hasAngles() const	{ return angledp_; }
    void			obtainGathers();
				/*!< Make all gathers available in the
				     FlatDataPack Mgr */

    ConstRefMan<PreStack::Gather> getGather(int trcnr,bool angles=false) const;

    ConstRefMan<FlatDataPack>	getTrcDP() const override
				{ return getTrcDPAtOffset(0); }
    ConstRefMan<FlatDataPack>	getTrcDPAtOffset(int offsidx) const;
    ConstRefMan<FlatDataPack>	getFlattenedTrcDP(const TypeSet<float>& zvals,
						  bool istime) const override
				{ return getFlattenedTrcDP(zvals,istime,0); }
    ConstRefMan<FlatDataPack>	getFlattenedTrcDP(const TypeSet<float>& zvals,
						 bool istime,int offsidx) const;

private:
				~PreStackSyntheticData();

    ConstRefMan<PreStack::GatherSetDataPack> angledp_;
    void				convertAngleDataToDegrees(
						PreStack::Gather&) const;
    static DataPack::MgrID		groupID();
};


mExpClass(WellAttrib) StratPropSyntheticData : public PostStackSyntheticData
{
public:
				StratPropSyntheticData(const SynthGenParams&,
						const Seis::SynthGenDataPack&,
						SeisTrcBufDataPack&,
						const PropertyRef&);

    bool			isStratProp() const override	{ return true; }

    const PropertyRef&		propRef() const { return prop_; }
    SynthGenParams::SynthType	synthType() const override
				{ return SynthGenParams::StratProp; }

private:
				~StratPropSyntheticData();

    const PropertyRef&		prop_;
};
