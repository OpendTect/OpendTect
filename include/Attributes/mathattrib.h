#ifndef mathattrib_h
#define mathattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attributesmod.h"
#include "attribprovider.h"

namespace Math { class Formula; class SpecVarSet; }

namespace Attrib
{

/*!
\brief %Math Attribute
*/

mExpClass(Attributes) Mathematics : public Provider
{
public:
    static void			initClass();
				Mathematics(Desc&);

    static const char*		attribName()		{ return "Math"; }
    static const char*		expressionStr()		{ return "expression"; }
    static const char*		cstStr()		{ return "constant"; }
    static const char*		recstartvalsStr()	{return "recstartvals";}

    static const Math::SpecVarSet&	getSpecVars();

protected:
				~Mathematics()	{}
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			getInputOutput(int in,TypeSet<int>& res) const;
    bool			getInputData(const BinID&, int);
    bool			computeData(const DataHolder&,const BinID& pos,
					    int t0,int nrsamples,
					    int threadid) const;

    bool			allowParallelComputation() const;

    const Interval<float>*	desZMargin(int input,int) const;
    const Interval<int>*        reqZSampMargin(int input,int) const;

private:
    ObjectSet<const DataHolder>	inputdata_;
    TypeSet<int>		inputidxs_;

    ::Math::Formula*		formula_;
    Interval<float>		desintv_;
    Interval<int>		reqintv_;

};

}; // namespace Attrib

#endif

