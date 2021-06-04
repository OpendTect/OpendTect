#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct 2006
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

    static Provider*		createInstance(Desc&);

    float			period_;
    Interval<int>		dessamps_;

    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID&,int,int,int) const;
    const Interval<int>*	desZSampMargin(int,int) const;

    const DataHolder*		refcubedata_;
    const DataHolder*		deltacubedata_;
    ValueSeries<float>*		refseries_;
    int				dcdataidx_;

};

} // namespace Attrib


