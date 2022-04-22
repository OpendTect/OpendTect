#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
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
    			BinIDSurfaceSourceSelector(const EM::Horizon3D&,
			       			   const EM::SectionID&);
    int			nextStep() override { return 0; }

protected:
    const EM::Horizon3D&	surface_;
};


/*!
\brief Surface SectionSourceSelector
*/

mExpClass(MPEEngine) SurfaceSourceSelector : public SectionSourceSelector
{
public:
    		SurfaceSourceSelector(const EM::EMObject&,
				      const EM::SectionID&);
    int		nextStep() override { return 0; }

protected:
    const EM::EMObject&	emobject_;
};


} // namespace MPE

