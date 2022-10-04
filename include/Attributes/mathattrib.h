#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
				~Mathematics();
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			getInputOutput(int in,
					    TypeSet<int>& res) const override;
    bool			getInputData(const BinID&, int) override;
    bool			computeData(const DataHolder&,const BinID& pos,
					    int t0,int nrsamples,
					    int threadid) const override;

    bool			allowParallelComputation() const override;

    const Interval<float>*	desZMargin(int input,int) const override;
    const Interval<int>*	reqZSampMargin(int input,int) const override;

private:
    ObjectSet<const DataHolder>	inputdata_;
    TypeSet<int>		inputidxs_;

    ::Math::Formula*		formula_;
    Interval<float>		desintv_;
    Interval<int>		reqintv_;

};

} // namespace Attrib
