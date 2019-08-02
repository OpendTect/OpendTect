#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		September 2007
________________________________________________________________________

-*/

#include "generalmod.h"
#include "trckeyzsampling.h"
#include "paralleltask.h"

class ZAxisTransform;
class DataPointSet;
class BinnedValueSet;

/*!
\brief Generates a DataPointSet with untransformed z-values corresponding to
each BinID and z-value of a specified TrcKeyZSampling.
*/

mExpClass(General) ZAxisTransformPointGenerator : public ParallelTask
{
public:
				ZAxisTransformPointGenerator(ZAxisTransform&);
				~ZAxisTransformPointGenerator();

    void			setInput(const TrcKeyZSampling&,
					 const TaskRunnerProvider&);
    void			setOutputDPS( DataPointSet& dps )
				{ dps_ = &dps; }

protected:

    bool			doPrepare(int nrthreads);
    bool			doWork(od_int64,od_int64,int threadid);
    bool			doFinish(bool success);
    od_int64			nrIterations() const
				{ return tkzs_.hsamp_.totalNr(); }

    int				voiid_;
    ObjectSet<BinnedValueSet>	bidvalsets_;
    TrcKeySamplingIterator	iter_;
    TrcKeyZSampling		tkzs_;
    ZAxisTransform&		transform_;
    DataPointSet*		dps_;

};
