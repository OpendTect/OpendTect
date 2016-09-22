#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		Sep 2016
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"
#include "syntheticdata.h"

class SeisTrcBuf;
namespace PreStack
{
class GatherSetDataPack;
class Gather;

mExpClass(PreStackProcessing) PreStackSyntheticData : public SyntheticData
{
public:
				PreStackSyntheticData(const SynthGenParams&,
						PreStack::GatherSetDataPack&);
				~PreStackSyntheticData();

    bool				isPS() const	  { return true; }
    bool				isNMOCorrected() const;
    bool				hasOffset() const;
    const Interval<float>		offsetRange() const;
    float				offsetRangeStep() const;
    SynthGenParams::SynthType		synthType() const
					{ return SynthGenParams::PreStack; }

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

    RefMan<PreStack::GatherSetDataPack> angledp_;
    void				convertAngleDataToDegrees(
						PreStack::Gather*) const;
};
} // namespace PreStack
