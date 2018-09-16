#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		Sep 2016
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"
#include "synthseisdataset.h"

class SeisTrcBuf;
class Gather;
class GatherSetDataPack;


namespace SynthSeis
{

mExpClass(PreStackProcessing) PreStackDataSet : public DataSet
{
public:

    typedef ObjectSet<Gather>	GatherSet;

				PreStackDataSet(const GenParams&,
						GatherSetDataPack&);
				~PreStackDataSet();

    bool			isPS() const	  { return true; }
    bool			isNMOCorrected() const;
    bool			hasOffset() const;
    const Interval<float>	offsetRange() const;
    float			offsetRangeStep() const;

    void			setAngleData(const GatherSet&);
    const SeisTrc*		getTrace(int seqnr,int offset) const;
    SeisTrcBuf*			getTrcBuf(float startoffset,
					  const Interval<float>* of=0) const;

    GatherSetDataPack&		preStackPack();
    const GatherSetDataPack&	preStackPack() const;
    const GatherSetDataPack&	angleData() const { return *angledp_; }

protected:

    RefMan<GatherSetDataPack>	angledp_;
    void			convertAngleDataToDegrees(Gather*) const;

    virtual const SeisTrc*	gtTrace(int seqnr) const
				{ return getTrace(seqnr,0); }
    virtual DataPackMgr::ID	dpMgrID() const;

};

} // namespace SynthSeis
