#ifndef prestackattrib_h
#define prestackattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        B.Bril & H.Huck
 Date:          14-01-2008
 RCS:           $Id: prestackattrib.h,v 1.5 2008-08-26 09:57:00 cvshelene Exp $
________________________________________________________________________

-*/


#include "attribprovider.h"
#include "seispsprop.h"
#include "multiid.h"
class SeisPSReader;


namespace Attrib
{

/*!\brief "Pre-Stack Attribute"

PreStack calctype= axistype= lsqtype= offsaxis= valaxis= useazim= comp= aperture=

Outputs a standard attribute from pre-stack data.

Input:
0		Pre-Stack Data

Output:
0		Attribute
*/
    

class PreStack: public Provider
{
public:

    static void		initClass();

			PreStack(Desc&);

    static const char*  attribName()		{ return "PreStack"; }
    static const char*  offStartStr()		{ return "offstart"; }
    static const char*  offStopStr()		{ return "offstop"; }
    static const char*  calctypeStr()		{ return "calctype"; }
    static const char*  stattypeStr()		{ return "stattype"; }
    static const char*  lsqtypeStr()		{ return "lsqtype"; }
    static const char*  offsaxisStr()		{ return "offsaxis"; }
    static const char*  valaxisStr()		{ return "valaxis"; }
    static const char*  useazimStr()		{ return "useazim"; }
    static const char*  componentStr()		{ return "comp"; }
    static const char*  apertureStr()		{ return "aperture"; }

    const SeisPSPropCalc::Setup& setup() const	{ return setup_; }
    const MultiID		psID() const	{ return psid_; }

protected:

			~PreStack();
    static Provider*    createInstance(Desc&);

    bool		allowParallelComputation() const	{ return false;}
    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&, int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;
    void		prepPriorToBoundsCalc();

    MultiID		psid_;
    Interval<float>	offsrg_;
    SeisPSPropCalc::Setup setup_;
    SeisPSReader*	psrdr_;
    SeisPSPropCalc*	propcalc_;
    int			dataidx_;
    const DataHolder*	inputdata_;
};

}; // namespace Attrib

#endif
