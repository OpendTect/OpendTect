#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Khushnood
 Date:		June 2019
________________________________________________________________________

-*/

#include "expattribsmod.h"
#include "attribprovider.h"
#include "arrayndalgo.h"
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
