#ifndef energyattrib_h
#define energyattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id$
________________________________________________________________________

-*/


#include "attributesmod.h"
#include "attribprovider.h"


namespace Attrib
{

/*!
\ingroup Attributes
\brief Energy Attribute

Calculates the squared sum of the gate's samples divided by the number of
samples in the gate.

\code
Energy gate= dograd=

Input:
0		Data

Outputs:	
0		The energy
1		Square root of the energy
2		Ln of the energy

if Gradient is selected outputs will be : grad(Energy), grad(SQRT(Energy)), ...
\endcode
*/

mClass(Attributes) Energy: public Provider
{
public:
    static void		initClass();
			Energy(Desc&);

    static const char*  attribName()		{ return "Energy"; }
    static const char*  gateStr()		{ return "gate"; }
    static const char*  dogradStr()		{ return "dograd"; }

protected:
    			~Energy() {}
    static Provider*    createInstance(Desc&);
    static void         updateDefaults(Desc&);

    bool		allowParallelComputation() const;
    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&, int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;

    const Interval<float>* reqZMargin(int input,int output) const
    			   { return &gate_; }
    const Interval<int>* desZSampMargin(int input,int output) const
    			   { return &dessampgate_; }
    
    Interval<float>	gate_;
    Interval<int>	dessampgate_;
    bool		dograd_;
    int			dataidx_;
    const DataHolder*	inputdata_;
};

} // namespace Attrib


#endif

