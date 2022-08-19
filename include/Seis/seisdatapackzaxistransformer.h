#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisdatapack.h"
#include "paralleltask.h"

class ZAxisTransform;

/*!
\brief Uses ZAxisTransform to output a transformed SeisDataPack for the
specified input SeisDataPack.
*/

mExpClass(Seis) SeisDataPackZAxisTransformer : public ParallelTask
{
public:
				SeisDataPackZAxisTransformer(ZAxisTransform&,
						SeisDataPack* outputdp=nullptr);
				~SeisDataPackZAxisTransformer();

    void			setInput( const SeisDataPack* dp )
				{ inputdp_ = dp; }
    void			setOutputZRange( const ZSampling& zrg )
				{ zrange_ = zrg; }
    void			setInterpolate( bool yn )
				{ interpolate_ = yn; }

    RefMan<SeisDataPack>	getOutput() { return outputdp_; }

protected:

    bool			doPrepare(int nrthreads) override;
    bool			doWork(od_int64,od_int64,int threadid) override;
    bool			doFinish(bool success) override;
    od_int64			nrIterations() const override;

    bool			interpolate_ = true;
    DataPackMgr&		dpm_;
    ZAxisTransform&		transform_;
    ZSampling			zrange_;

    ConstRefMan<DataPack>	inputdp_;
    RefMan<SeisDataPack>	outputdp_;
};
