#ifndef attribdatapackzaxistransformer_h
#define attribdatapackzaxistransformer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		June 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "attribdatapack.h"
#include "paralleltask.h"

class ZAxisTransform;

namespace Attrib
{

/*!
\brief Transforms datapacks using ZAxisTransform. Output datapack will be of
same type as inputdp_. At present, this works only for Flat2DDHDataPack and
FlatRdmTrcsDataPack.
*/

mExpClass(AttributeEngine) FlatDataPackZAxisTransformer : public ParallelTask
{
public:
				FlatDataPackZAxisTransformer(ZAxisTransform&);
				~FlatDataPackZAxisTransformer();

    void			setInput( const FlatDataPack* fdp )
				{ inputdp_ = fdp ? dpm_.obtain(fdp->id()) : 0; }
    void			setOutput( DataPack::ID& dpid )
				{ outputid_ = &dpid; dpid = DataPack::cNoID(); }
    void			setInterpolate( bool yn )
				{ interpolate_ = yn; }

protected:

    bool			doPrepare(int nrthreads);
    bool			doWork(od_int64,od_int64,int threadid);
    bool			doFinish(bool success);
    od_int64			nrIterations() const;

    bool			interpolate_;
    DataPackMgr&		dpm_;
    ZAxisTransform&		transform_;
    StepInterval<float>		zrange_;

    ConstDataPackRef<FlatDataPack>	inputdp_;
    DataPack::ID*			outputid_;
    PtrMan<Array2D<float> >		arr2d_;
};


} // namespace Attrib

#endif
