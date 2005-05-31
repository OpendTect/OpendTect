#ifndef mathattrib_h
#define mathattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2005
 RCS:           $Id: mathattrib.h,v 1.1 2005-05-31 07:16:06 cvsnanne Exp $
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
    static Provider*		internalCreate(Desc&,ObjectSet<Provider>& exis);

    bool			getInputOutput(int in,TypeSet<int>& res) const;
    bool			getInputData(const BinID&);
    bool			computeData(const DataHolder&,const BinID& pos,
	    				    int t0,int nrsamples) const;

    ObjectSet<const DataHolder>	inputdata_;

private:
    TypeSet<int>		inputtable_;
    MathExpression*		expression_;
};

}; // namespace Attrib

#endif
