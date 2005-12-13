#ifndef evaluateattrib_h
#define evaluateattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          19-10-2005
 RCS:           $Id: evaluateattrib.h,v 1.2 2005-12-13 10:03:41 cvshelene Exp $
________________________________________________________________________

-*/


#include "attribprovider.h"


/*!\brief Evaluate Attribute

Attribute built for convenience purpose; only there to allow the computation of
a set of attributes in one go;

*/
    

namespace Attrib
{

class Evaluate: public Provider
{
public:
    static void		initClass();
			Evaluate(Desc&);

    static const char*  attribName()		{ return "Evaluate"; }

protected:
    static Provider*    createInstance(Desc&);

    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&, int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples) const;

    TypeSet<int>		dataidx_;
    ObjectSet<const DataHolder>	inputdata;
};

}; // namespace Attrib


#endif
