#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisdatapack.h"

#include "paralleltask.h"
#include "zaxistransform.h"


/*!
\brief Uses ZAxisTransform to output a transformed SeisVolumeDataPack for the
specified input SeisVolumeDataPack.
*/

mExpClass(Seis) SeisDataPackZAxisTransformer : public ParallelTask
{
public:
				SeisDataPackZAxisTransformer(ZAxisTransform&,
					SeisVolumeDataPack* outputdp=nullptr);
				~SeisDataPackZAxisTransformer();

    void			setInput(const SeisVolumeDataPack*);
    void			setOutputZRange( const ZSampling& zrg )
				{ zrange_ = zrg; }
    void			setInterpolate( bool yn )
				{ interpolate_ = yn; }

    RefMan<SeisVolumeDataPack>	getOutput() { return outputdp_; }

protected:

    bool			doPrepare(int nrthreads) override;
    bool			doWork(od_int64,od_int64,int threadid) override;
    bool			doFinish(bool success) override;
    od_int64			nrIterations() const override;

    bool			interpolate_ = true;
    ConstRefMan<ZAxisTransform> transform_;
    ZSampling			zrange_;

    ConstRefMan<SeisVolumeDataPack> inputdp_;
    RefMan<SeisVolumeDataPack>	outputdp_;
};
