#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributesmod.h"
#include "attribprovider.h"


namespace Attrib
{

/*!
\brief %Energy Attribute

Calculates the squared sum of the gate's samples divided by the number of
samples in the gate.

<pre>
Energy gate= dograd=

Input:
0		Data

Outputs:	
0		The energy
1		Square root of the energy
2		Ln of the energy

if Gradient is selected outputs will be : grad(Energy), grad(SQRT(Energy)), ...
</pre>
*/

mExpClass(Attributes) Energy: public Provider
{
public:
    static void		initClass();
			Energy(Desc&);

    static const char*	attribName()		{ return "Energy"; }
    static const char*	gateStr()		{ return "gate"; }
    static const char*	dogradStr()		{ return "dograd"; }

protected:
			~Energy() {}
    static Provider*	createInstance(Desc&);
    static void		updateDefaults(Desc&);

    bool		allowParallelComputation() const override;
    bool		getInputOutput(int input,
				       TypeSet<int>& res) const override;
    bool		getInputData(const BinID&, int idx) override;
    bool		computeData(const DataHolder&,const BinID& relpos,
			    int t0,int nrsamples,int threadid) const override;

    const Interval<float>* reqZMargin(int input,int output) const override
			   { return &gate_; }
    const Interval<int>* desZSampMargin(int input,int output) const override
			   { return &dessampgate_; }
    
    Interval<float>	gate_;
    Interval<int>	dessampgate_;
    bool		dograd_;
    int			dataidx_;
    const DataHolder*	inputdata_;
};

} // namespace Attrib
