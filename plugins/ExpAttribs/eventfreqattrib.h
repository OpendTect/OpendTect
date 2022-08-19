#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "expattribsmod.h"
#include "attribprovider.h"
#include "valseries.h"

/*!\brief Computes a simple frequency from distance between events */

namespace Attrib
{

mClass(ExpAttribs) EventFreq : public Provider
{
public:

    static void		initClass();
			EventFreq(Desc&);

    static const char*	attribName()	   { return "EventFreq"; }

protected:

    static Provider*	createInstance(Desc&);

    bool		getInputData(const BinID&,int);
    bool		computeData(const DataHolder&,const BinID&,
				    int,int,int) const;

    const DataHolder*	inpdata_;
    ValueSeries<float>*	inpseries_;
    Interval<int>	dessamps_;
    Interval<int>	cubeintv_;

    mutable TypeSet<float> evposns_;
    mutable bool	firstevmax_;

    void		findEvents(int,int) const;
    void		fillFreqOutput(const DataHolder&,int,int) const;
    void		fillPhaseOutput(const DataHolder&,int,int) const;

    const Interval<int>* desZSampMargin(int,int) const;
    float		getPDz(float*,int) const;

};

} // namespace Attrib
