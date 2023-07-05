#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "datapointset.h"
#include "paralleltask.h"
#include "trckeyzsampling.h"

class BinIDValueSet;
class ZAxisTransform;

/*!
\brief Generates a DataPointSet with untransformed z-values corresponding to
each BinID and z-value of a specified TrcKeyZSampling.
*/

mExpClass(General) ZAxisTransformPointGenerator : public ParallelTask
{
public:
				ZAxisTransformPointGenerator(ZAxisTransform&);
				~ZAxisTransformPointGenerator();

    void			setInput(const TrcKeyZSampling& cs,
					 TaskRunner* =0);
    void			setOutputDPS( DataPointSet& dps )
				{ dps_ = &dps; }

protected:

    bool			doPrepare(int nrthreads) override;
    bool			doWork(od_int64,od_int64,int threadid) override;
    bool			doFinish(bool success) override;
    od_int64			nrIterations() const override
				{ return tkzs_.hsamp_.totalNr(); }

    int 			voiid_;
    ObjectSet<BinIDValueSet>	bidvalsets_;
    TrcKeySamplingIterator 	iter_;
    TrcKeyZSampling		tkzs_;
    ZAxisTransform&		transform_;
    RefMan<DataPointSet>	dps_;
};
