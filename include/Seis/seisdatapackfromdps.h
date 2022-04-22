#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		November 2017
________________________________________________________________________

-*/

#include "seismod.h"

#include "paralleltask.h"

class BufferStringSet;
class DataPointSet;
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
				~SeisDataPackFromDPS()	{}
protected:

    bool			doWork(od_int64 start,od_int64 stop,
							int threadidx) override;
    bool			doPrepare(int nrthreads) override;
    od_int64			nrIterations() const override;

    TypeSet<int>		selcols_;
    TypeSet<int>		selcomps_;
    const DataPointSet&		dps_;
    RegularSeisDataPack&		dp_;

};

