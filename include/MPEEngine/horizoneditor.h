#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2005
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "emeditor.h"

#include "bufstringset.h"

namespace EM { class Horizon3D; class Horizon2D; }

namespace MPE
{

/*!
\brief ObjectEditor to edit EM::Horizon3D.
*/

mExpClass(MPEEngine) HorizonEditor : public ObjectEditor
{
public:
    				HorizonEditor(EM::Horizon3D&);
    static ObjectEditor*	create(EM::EMObject&);
    static void			initClass();

    void			getEditIDs(TypeSet<EM::PosID>&) const override;

protected:
    Geometry::ElementEditor*	createEditor(const EM::SectionID&) override;
};


/*!
\brief ObjectEditor to edit EM::Horizon2D.
*/

mExpClass(MPEEngine) Horizon2DEditor : public ObjectEditor
{
public:
				Horizon2DEditor(EM::Horizon2D&);
    static ObjectEditor*	create(EM::EMObject&);
    static void			initClass();

protected:
    Geometry::ElementEditor*	createEditor(const EM::SectionID&) override;
};


} // namespace MPE

