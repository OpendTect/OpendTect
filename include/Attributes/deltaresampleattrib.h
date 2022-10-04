#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribprovider.h"
template <class T> class ValueSeries;

namespace Attrib
{

/*!
\brief Resamples the trace at shifted locations.
*/

mClass(Attributes) DeltaResample : public Provider
{
public:

    static void			initClass();
				DeltaResample(Desc&);

    static const char*		attribName()	   { return "DeltaResample"; }
    static const char*		periodStr()	   { return "period"; }

protected:
				~DeltaResample();
    static Provider*		createInstance(Desc&);

    float			period_;
    Interval<int>		dessamps_;

    bool			getInputData(const BinID&,int zintv) override;
    bool			computeData(const DataHolder&,
				    const BinID&,int,int,int) const override;
    const Interval<int>*	desZSampMargin(int,int) const override;

    const DataHolder*		refcubedata_;
    const DataHolder*		deltacubedata_;
    ValueSeries<float>*		refseries_;
    int				dcdataidx_;

};

} // namespace Attrib
