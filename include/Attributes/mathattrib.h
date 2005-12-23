#ifndef mathattrib_h
#define mathattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2005
 RCS:           $Id: mathattrib.h,v 1.5 2005-12-23 16:09:46 cvsnanne Exp $
________________________________________________________________________

-*/

/*! \brief
#### Short description
\par
#### Detailed description.

*/

#include "attribprovider.h"

class MathExpression;

namespace Attrib
{

class Math : public Provider
{
public:
    static void			initClass();
    				Math(Desc&);

    static const char*		attribName()		{ return "Math"; }
    static const char*		expressionStr()		{ return "expression"; }

protected:
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			getInputOutput(int in,TypeSet<int>& res) const;
    bool			getInputData(const BinID&, int);
    bool			computeData(const DataHolder&,const BinID& pos,
	    				    int t0,int nrsamples) const;

    bool			allowParallelComputation() const
    				{ return true; }

private:
    ObjectSet<const DataHolder>	inputdata_;
    TypeSet<int>		inputidxs_;
    TypeSet<int>		inputtable_;
    MathExpression*		expression_;
};

}; // namespace Attrib

#endif
