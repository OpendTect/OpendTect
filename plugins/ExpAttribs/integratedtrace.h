#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "expattribsmod.h"
#include "attribprovider.h"

namespace Attrib
{

mExpClass(ExpAttribs) IntegratedTrace : public Provider
{
public:
    static void			initClass();
				IntegratedTrace(Desc&);

    static const char*		attribName()	{ return "IntegratedTrace"; }

    static const char*		gateStr()		{ return "gate"; }

protected:

    static Provider*		createInstance(Desc&);

    bool			allowParallelComputation() const;
    bool			getInputData(const BinID&,int);
    bool			computeData(const DataHolder&,
					    const BinID&,int,int,int) const;

    const Interval<int>*	desZSampMargin(int input,int output) const;

    const DataHolder*		inputdata_;
    int				dataidx_;
    Interval<int>		desgate_;
};

} // namespace Attrib
