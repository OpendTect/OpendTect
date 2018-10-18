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

class RayModelSet;

mExpClass(PreStackProcessing) PreStackDataSet : public DataSet
{
public:

    typedef ObjectSet<Gather>	GatherSet;

				PreStackDataSet(const GenParams&,
						GatherSetDataPack&,
						RayModelSet&);
				~PreStackDataSet();

    virtual bool		isPS() const	  { return true; }
    bool			isNMOCorrected() const;
    int				nrOffsets() const;
    virtual OffsetDef		offsetDef() const;
    Interval<float>		offsetRange() const;
    float			offsetStep() const;

    void			setAngleData(const GatherSet&);
    const SeisTrc*		getTrc(int seqnr,int offset) const;
    SeisTrcBuf*			getTrcBuf(float startoffset,
					  const Interval<float>* of=0) const;

    GatherSetDataPack&		preStackPack();
    const GatherSetDataPack&	preStackPack() const;
    const GatherSetDataPack&	angleData() const { return *angledp_; }

protected:

    typedef ObjectSet<SeisTrc>	TrcSet;

    RefMan<GatherSetDataPack>	angledp_;
    mutable ObjectSet<TrcSet>	trccache_;

    void			ensureCacheReady() const;
    const SeisTrc*		getCachedTrc(int seqnr,int offsnr) const;
    SeisTrc*			createTrc(int seqnr,int ioffs) const;
    const SeisTrc*		addTrcToCache(int seqnr,int offsnr) const;

    virtual DataPackMgr::ID	dpMgrID() const;
    virtual const SeisTrc*	gtTrc(int,float) const;

};

} // namespace SynthSeis
