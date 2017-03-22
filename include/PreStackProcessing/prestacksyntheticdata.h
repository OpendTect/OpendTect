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

class GatherSetDataPack;
class Gather;
class SeisTrcBuf;

namespace PreStack
{

mExpClass(PreStackProcessing) PreStackSyntheticData : public SyntheticData
{
public:
				PreStackSyntheticData(const SynthGenParams&,
						GatherSetDataPack&);
				~PreStackSyntheticData();

    bool				isPS() const	  { return true; }
    bool				isNMOCorrected() const;
    bool				hasOffset() const;
    const Interval<float>		offsetRange() const;
    float				offsetRangeStep() const;
    SynthGenParams::SynthType		synthType() const
					{ return SynthGenParams::PreStack; }

    void				setAngleData(
					    const ObjectSet<Gather>&);
    const SeisTrc*			getTrace(int seqnr) const
					{ return getTrace(seqnr,0); }
    const SeisTrc*			getTrace(int seqnr,int* offset) const;
    SeisTrcBuf*				getTrcBuf(float startoffset,
					    const Interval<float>* of=0) const;

    GatherSetDataPack&	preStackPack();
    const GatherSetDataPack&	preStackPack() const;
    const GatherSetDataPack&	angleData() const { return *angledp_; }

protected:

    RefMan<GatherSetDataPack> angledp_;
    void				convertAngleDataToDegrees(
						Gather*) const;
};
} // namespace PreStack
