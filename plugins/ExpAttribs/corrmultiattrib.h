#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "expattribsmod.h"
#include "attribprovider.h"

/*!\brief Tutorial Attribute

Input:
0		Data

Outputs:
0		The scaled trace
*/


namespace Attrib
{

mExpClass(ExpAttribs) CorrMultiAttrib : public Provider
{
public:

    static void			initClass();
				CorrMultiAttrib(Desc&);
    static const char*		gateStr() { return "gate"; }
    static const char*		attribName() {return "Multiattrib_Similarity";}


protected:

				~CorrMultiAttrib() {}
    static Provider*		createInstance(Desc&);
    static void			updateDefaults(Desc&);

    bool			getInputOutput(int input,
						TypeSet<int>& res) const;
    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
					     const BinID& relpos,int z0,
					     int nrsamples,int threadid)
					     const;

    const Interval<float>*	desZMargin(int input,int output) const;

    Interval<float>		gate_;
    ObjectSet<const DataHolder> inpdata_;
    int				dataidx1_;
    int				dataidx2_;

    const DataHolder*		inputdata1_;
    const DataHolder*		inputdata2_;
};

} // namespace Attrib
