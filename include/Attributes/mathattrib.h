#ifndef mathattrib_h
#define mathattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2005
 RCS:           $Id: mathattrib.h,v 1.9 2006-10-24 15:21:36 cvshelene Exp $
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
    static const char*		cstStr()		{ return "constant"; }

    static void 		getInputTable(const MathExpression*,
					      TypeSet<int>&,bool);

protected:
    				~Math()	{}
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
    TypeSet<int>		varsinputtable_;
    TypeSet<int>		cstsinputtable_;
    TypeSet<float>		csts_;
    MathExpression*		expression_;
};

}; // namespace Attrib

#endif
