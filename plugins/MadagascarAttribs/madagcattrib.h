#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "madagascarattribsmod.h"
#include "attribprovider.h"
template <class T> class ValueSeries;

/*!\brief uses automatic gain control to scale traces.
*/

namespace Attrib
{

mClass(MadagascarAttribs) MadAGC : public Provider
{
public:

    static void			initClass();
				MadAGC(Desc&);

    static StringView		attribName()	   { return "MadagascarAGC"; }
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
