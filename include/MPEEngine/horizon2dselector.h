#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "sectionselector.h"

namespace EM { class Horizon2D; }

namespace MPE
{

/*!
\brief SectionSourceSelector to select EM::Horizon2D.
*/

mExpClass(MPEEngine) Horizon2DSelector : public SectionSourceSelector
{
public:
				Horizon2DSelector(const EM::Horizon2D&);
    mDeprecated("Use without SectionID")
				Horizon2DSelector(const EM::Horizon2D& hor2d,
						  const EM::SectionID&)
				    : Horizon2DSelector(hor2d)	{}
    int				nextStep() override;

protected:
    const EM::Horizon2D&	horizon_;
};

} // namespace MPE
