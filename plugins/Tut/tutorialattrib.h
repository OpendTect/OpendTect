#ifndef tutorialattrib_h
#define tutorialattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          May 2007
 RCS:           $Id: tutorialattrib.h,v 1.1 2007-06-01 06:30:59 cvsraman Exp $
________________________________________________________________________

-*/


#include "attribprovider.h"

/*!\brief Tutorial Attribute

Input:
0               Data

Outputs:
0               The scaled trace
*/
    

namespace Attrib
{

class Tutorial : public Provider
{
public:
    static void		initClass();
			Tutorial(Desc&);
    static const char*	attribName()		{ return "Tutorial"; }
    static const char*	actionStr()		{ return "action"; }
    static const char*	shiftStr()		{ return "shift"; }
    static const char*	factorStr()		{ return "factor"; }
    static const char*	smoothStr()		{ return "smoothtype"; }

protected:
			~Tutorial() {}
    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);

    bool		allowParallelComputation() const	{ return true; }

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&,int zintv);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int z0,int nrsamples,int threadid) const;

    int			action_;
    float		factor_;
    float		shift_;
    bool		weaksmooth_;
    const DataHolder*	inputdata_;
    int			dataidx_;
};

}; // namespace Attrib

#endif
