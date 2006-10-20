#ifndef energyattrib_h
#define energyattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: energyattrib.h,v 1.9 2006-10-20 19:43:15 cvskris Exp $
________________________________________________________________________

-*/


#include "attribprovider.h"


namespace Attrib
{

/*!\brief "Energy Attribute"

Energy gate=

Calculates the squared sum of the gate's samples divided by the number of
samples in the gate.

Input:
0		Data

Outputs:
0		The energy
1		Square root of the energy
2		Ln of the energy
*/
    

class Energy: public Provider
{
public:
    static void		initClass();
			Energy(Desc&);

    static const char*  attribName()		{ return "Energy"; }
    static const char*  gateStr()		{ return "gate"; }

protected:
    			~Energy() {}
    static Provider*    createInstance(Desc&);

    bool		allowParallelComputation() const	{ return true; }
    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&, int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples) const;

    const Interval<float>* reqZMargin(int input,int output) const
    			   { return &gate_; }
    
    Interval<float>	gate_;
    int			dataidx_;
    const DataHolder*	inputdata_;
};

}; // namespace Attrib


/*!\mainpage Standard Attributes

  This module contains the definition of the 'standard' OpendTect attributes.
  Contained are attributes like Energy, Similarity, Volume Statistics, etc.
  The base class for all attributes is the Provider class.

  The Attribute factories are defined in the Attribute Engine module
  (AttributeEngine).

  If you want to make your own attributes, please consult the Programmer's
  manual, section 'Plugins'. You'll find an annotated example of the Coherency
  attribute implementation.

*/


#endif
