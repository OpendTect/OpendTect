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
\brief Sample Value Attribute

  Shortcut for dummies (like us) to Shift or Mathematics or ... attribute's
  functionality.
*/

mExpClass(Attributes) SampleValue : public Provider
{
public:
    static void			initClass();
				SampleValue(Desc&);

    static const char*		attribName()	{ return "SampleValue"; }

protected:
				~SampleValue();
    static Provider*		createInstance(Desc&);

    bool			allowParallelComputation() const override;
    bool			getInputData(const BinID&,int) override;
    bool			computeData(const DataHolder&,
				    const BinID&,int,int,int) const override;

    const DataHolder*		inputdata_;
    int				dataidx_;

};

} // namespace Attrib
