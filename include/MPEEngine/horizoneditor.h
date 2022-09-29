#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
				~HorizonEditor();

    Geometry::ElementEditor*	createEditor() override;
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
				~Horizon2DEditor();

    Geometry::ElementEditor*	createEditor() override;
};


} // namespace MPE
