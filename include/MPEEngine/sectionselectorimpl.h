#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "sectionselector.h"

namespace EM { class Horizon3D; class EMObject; }


namespace MPE
{

/*!
\brief BinID SectionSourceSelector.
*/

mExpClass(MPEEngine) BinIDSurfaceSourceSelector : public SectionSourceSelector
{
public:
			BinIDSurfaceSourceSelector(const EM::Horizon3D&);
			~BinIDSurfaceSourceSelector();

    int			nextStep() override;

protected:
    const EM::Horizon3D&	surface_;
};


/*!
\brief Surface SectionSourceSelector
*/

mExpClass(MPEEngine) SurfaceSourceSelector : public SectionSourceSelector
{
public:
			SurfaceSourceSelector(const EM::EMObject&);
			~SurfaceSourceSelector();

    int			nextStep() override;

protected:
    const EM::EMObject&	emobject_;
};

} // namespace MPE
