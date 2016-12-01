#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		June 2014
________________________________________________________________________

-*/

#include "generalmod.h"
#include "datapackbase.h"
#include "paralleltask.h"

class ZAxisTransform;

/*!
\brief Uses ZAxisTransform to output a transformed VolumeDataPack for the
specified input VolumeDataPack.
*/

mExpClass(General) VolumeDataPackZAxisTransformer : public ParallelTask
{
public:
				VolumeDataPackZAxisTransformer(ZAxisTransform&,
						VolumeDataPack* outputdp=0);
				~VolumeDataPackZAxisTransformer();

    void			setInput( const VolumeDataPack* dp )
				{ inputdp_ = dp; }
    void			setOutputZRange( const StepInterval<float>& zrg)
				{ zrange_ = zrg; }
    void			setInterpolate( bool yn )
				{ interpolate_ = yn; }

    RefMan<VolumeDataPack>	getOutput() { return outputdp_; }

protected:

    bool			doPrepare(int nrthreads);
    bool			doWork(od_int64,od_int64,int threadid);
    bool			doFinish(bool success);
    od_int64			nrIterations() const;

    bool			interpolate_;
    DataPackMgr&		dpm_;
    ZAxisTransform&		transform_;
    StepInterval<float>		zrange_;

    ConstRefMan<DataPack>	inputdp_;
    RefMan<VolumeDataPack>	outputdp_;
};
