#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2002
________________________________________________________________________

-*/

#include "trckeyzsampling.h"


mExpClass(Basic) TrcKeyCollection
{
public:
    virtual od_int64		nrTrcs() const				= 0;
    virtual TrcKey		getTrcKey(od_int64 globaltrcidx)	= 0;
    virtual od_int64		getGlobalIdx(const TrcKey&) const	= 0;
    const OffsetValueSeries	getData(od_int64 globaltrcidx) const;
    const StepInterval<float>	getZInterval(od_int64 globaltrcidx) const = 0;
};


mExpClass(Basic) SampledTrcKeyCollection : public TrcKeyCollection
{
    TrcKeyZSampling		sampling_;
};


mExpClass(Basic) TableTrcKeySelection : public TrcKeySelection
{
    BinIDValueSet		table_;
};
