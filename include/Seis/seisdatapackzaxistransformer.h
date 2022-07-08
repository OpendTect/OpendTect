#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		June 2014
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
				SeisDataPackZAxisTransformer(ZAxisTransform&);
				~SeisDataPackZAxisTransformer();

    void			setInput( const SeisDataPack* dp )
			    { inputdp_ = dp ? dpm_.getDP(dp->id()) : nullptr; }
    void			setOutput( DataPack::ID& dpid )
				{ outputid_ = &dpid; dpid = DataPack::cNoID(); }
    void			setOutputZRange( const StepInterval<float>& zrg)
				{ zrange_ = zrg; }
    void			setInterpolate( bool yn )
				{ interpolate_ = yn; }

protected:

    bool			doPrepare(int nrthreads) override;
    bool			doWork(od_int64,od_int64,int threadid) override;
    bool			doFinish(bool success) override;
    od_int64			nrIterations() const override;

    bool			interpolate_;
    DataPackMgr&		dpm_;
    ZAxisTransform&		transform_;
    StepInterval<float>		zrange_;

    ConstRefMan<DataPack>	inputdp_;
    SeisDataPack*		outputdp_;
    DataPack::ID*		outputid_;
};

