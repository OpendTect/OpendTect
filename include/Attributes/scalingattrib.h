#ifndef scalingattrib_h
#define scalingattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          December 2004
 RCS:           $Id: scalingattrib.h,v 1.10 2006-04-11 15:17:58 cvshelene Exp $
________________________________________________________________________

-*/


#include "attribprovider.h"

/*!\brief Scaling Attribute

  Scaling gate=

  Calculates the squared sum of the gate's samples divided by the number of
  samples in the gate.

Input:
0               Data

Outputs:
0               The scaled trace
*/
    

namespace Attrib
{

class Scaling: public Provider
{
public:
    static void		initClass();
			Scaling(Desc&);
    static const char*	attribName()		{ return "Scaling"; }
    static const char*	scalingTypeStr()	{ return "scalingtype"; }
    static const char*	powervalStr()		{ return "powerval"; }
    static const char*	gateStr()		{ return "timegate"; }
    static const char*	factorStr()		{ return "scalefactor"; }
    static const char*	statsTypeStr()		{ return "statstype"; }
    static const char*	statsTypeNamesStr(int type);
    static const char*	scalingTypeNamesStr(int type);

protected:

    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);

    bool		allowParallelComputation()	{ return true; }

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int zintv);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int z0,int nrsamples) const;

    void		getSampleGates(const TypeSet<Interval<float> >& oldtgs,
				       TypeSet< Interval<int> >& newsampgates,
				       int z0,int nrsamples) const;
    void		scaleTimeN(const DataHolder&, int, int) const;
    void		getScaleFactorsFromStats(
	    			const TypeSet<Interval<int> >& gates,
				TypeSet<float>& factors) const;
    const Interval<int>* desZSampMargin( int inp, int ) const;

    int			scalingtype_;
    int			statstype_;
    float		powerval_;
    TypeSet< Interval<float> >	gates_;
    TypeSet<float>	factors_;
    const DataHolder*	inputdata_;
    int			dataidx_;
    Interval<int>	desgate_;
};

}; // namespace Attrib

#endif
