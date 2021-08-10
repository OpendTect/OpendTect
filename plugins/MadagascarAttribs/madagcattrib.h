#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          Sep 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "madagascarattribsmod.h"
#include "attribprovider.h"
template <class T> class ValueSeries;

namespace Attrib
{
//!\brief Uses automatic gain control to scale traces.

mClass(MadagascarAttribs) MadAGC : public Provider
{
public:

    static void			initClass();
				MadAGC(Desc&);

    static FixedString		attribName()	   { return "MadagascarAGC"; }
    static const char*		nrrepeatStr()	   { return "nrrepeat"; }
    static const char*		smoothradiusStr()  { return "latradius"; }
    static const char*		smoothzradiusStr() { return "zradius"; }


protected:

    static Provider*		createInstance(Desc&);

    int				nrrepeat_;
    Interval<int>		dessamps_;
    BinID			reqstepout_;

    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,const BinID&,
	    				    int,int,int) const;
    const Interval<int>*	desZSampMargin(int,int) const;
    const BinID*                reqStepout(int input,int output) const;

    ObjectSet<const DataHolder> inputdata_;
};

} // namespace Attrib


