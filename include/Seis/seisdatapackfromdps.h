#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "datapointset.h"
#include "paralleltask.h"

class BufferStringSet;
class RegularSeisDataPack;

/*!
\brief Class that creates a SeisDataPack from a DataPointSeti.
Columns in DPS are represented by components in DP.
*/

mExpClass(Seis) SeisDataPackFromDPS : public ParallelTask
{ mODTextTranslationClass(SeisDataPackFromDPS);
public:

				SeisDataPackFromDPS(const DataPointSet&,
					RegularSeisDataPack&,
					const BufferStringSet& columns);
				~SeisDataPackFromDPS();
protected:

    bool			doWork(od_int64 start,od_int64 stop,
							int threadidx) override;
    bool			doPrepare(int nrthreads) override;
    od_int64			nrIterations() const override;

    TypeSet<int>		selcols_;
    TypeSet<int>		selcomps_;
    ConstRefMan<DataPointSet>	dps_;
    RegularSeisDataPack&	dp_;

};
