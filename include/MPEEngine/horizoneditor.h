#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "emeditor.h"

namespace EM { class Horizon2D; class Horizon3D; }

namespace MPE
{

/*!
\brief ObjectEditor to edit EM::Horizon3D.
*/

mExpClass(MPEEngine) HorizonEditor : public ObjectEditor
{
public:

    static RefMan<HorizonEditor> create(EM::Horizon3D&);

protected:
				~HorizonEditor();

private:
				HorizonEditor(const EM::Horizon3D&);

    void			getEditIDs(TypeSet<EM::PosID>&) const override;


    Geometry::ElementEditor*	createEditor() override;

};


/*!
\brief ObjectEditor to edit EM::Horizon2D.
*/

mExpClass(MPEEngine) Horizon2DEditor : public ObjectEditor
{
public:

    static RefMan<Horizon2DEditor> create(EM::Horizon2D&);

protected:
				~Horizon2DEditor();

private:
				Horizon2DEditor(const EM::Horizon2D&);

    Geometry::ElementEditor*	createEditor() override;

};


} // namespace MPE
