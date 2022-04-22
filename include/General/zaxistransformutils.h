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
class BinIDValueSet;

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
    DataPointSet*		dps_;
};

