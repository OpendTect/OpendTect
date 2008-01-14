#ifndef prestackattrib_h
#define prestackattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        B.Bril & H.Huck
 Date:          14-01-2008
 RCS:           $Id: prestackattrib.h,v 1.1 2008-01-14 15:59:44 cvshelene Exp $
________________________________________________________________________

-*/


#include "attribprovider.h"


namespace Attrib
{

/*!\brief "Pre-Stack Attribute"

PreStack subtype=

Outputs a standart attribute from pre-stack data.
Can be used as intermediate between pre-stack data and some other attribute
whom definition string is hold in the string parameter 'subtype'. 

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
    static const char*  subtypeStr()		{ return "subtype"; }

protected:
    			~PreStack() {}
    static Provider*    createInstance(Desc&);

    bool		allowParallelComputation() const	{ return false;}
    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&, int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;

    BufferString	subtypestr_;
    int			dataidx_;
    const DataHolder*	inputdata_;
};

}; // namespace Attrib

#endif
