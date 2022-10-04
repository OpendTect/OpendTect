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
\brief Calculates %Hilbert transform.
*/

mExpClass(Attributes) Hilbert : public Provider
{
public:
    static void			initClass();
				Hilbert(Desc&);

    static const char*		attribName()	{ return "Hilbert"; }
    static const char*		halflenStr()	{ return "halflen"; }

protected:
				~Hilbert();
    static Provider*		createInstance(Desc&);

    bool			getInputOutput(int inp,
					    TypeSet<int>& res) const override;
    bool			getInputData(const BinID&, int) override;
    bool			computeData(const DataHolder&,const BinID& pos,
					    int z0,int nrsamples,
					    int threadid) const override;

    bool			allowParallelComputation() const override
				{ return true; }
    const Interval<int>*	desZSampMargin(int input,
					       int output) const override
				{ return &zmargin_; }

    const DataHolder*		inputdata_;
    int				dataidx_;

    Interval<int>		zmargin_;
    int				halflen_;

    const float*		hilbfilter_;
};

} // namespace Attrib
