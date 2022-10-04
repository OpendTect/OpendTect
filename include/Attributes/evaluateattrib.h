#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attributesmod.h"
#include "attribprovider.h"


namespace Attrib
{

/*!
\brief %Evaluate Attribute
  
  Attribute built for convenience purpose; only there to allow the computation
  of a set of attributes in one go;
*/
    
mExpClass(Attributes) Evaluate : public Provider
{
public:
    static void			initClass();
				Evaluate(Desc&);

    static const char*		attribName()		{ return "Evaluate"; }

protected:
				~Evaluate();
    static Provider*		createInstance(Desc&);

    bool			allowParallelComputation() const override
				{ return true; }

    bool			getInputOutput(int inp,
					    TypeSet<int>& res) const override;
    bool			getInputData(const BinID&,int zintv) override;
    bool			computeData(const DataHolder&,
					    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const override;

    TypeSet<int>		dataidx_;
    ObjectSet<const DataHolder> inputdata_;
};

} // namespace Attrib
