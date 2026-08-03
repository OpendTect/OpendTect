/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "horizoneditor.h"

#include "binidsurface.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "geeditorimpl.h"
#include "mpeengine.h"

namespace MPE
{

// HorizonEditor

RefMan<HorizonEditor>
			HorizonEditor::create( EM::Horizon3D& hor )
{
    return new HorizonEditor( hor );
}


HorizonEditor::HorizonEditor( const EM::Horizon3D& hor3d )
    : ObjectEditor(hor3d)
{
}


HorizonEditor::~HorizonEditor()
{}


Geometry::ElementEditor* HorizonEditor::createEditor()
{
    RefMan<EM::EMObject> emobject = emObject();
    Geometry::Element* ge = emobject ? emobject->geometryElement() : nullptr;
    mDynamicCastGet(Geometry::BinIDSurface*,surface,ge);
    return surface ? new Geometry::BinIDElementEditor( *surface ) : nullptr;
}


void HorizonEditor::getEditIDs( TypeSet<EM::PosID>& ids ) const
{
    ids.erase();
}


// Horizon2DEditor

RefMan<Horizon2DEditor>
			Horizon2DEditor::create( EM::Horizon2D& hor )
{
    return new Horizon2DEditor( hor );
}


Horizon2DEditor::Horizon2DEditor( const EM::Horizon2D& hor2d )
    : ObjectEditor(hor2d)
{}


Horizon2DEditor::~Horizon2DEditor()
{}


Geometry::ElementEditor* Horizon2DEditor::createEditor()
{
    RefMan<EM::EMObject> emobject = emObject();
    Geometry::Element* ge = emobject ? emobject->geometryElement() : nullptr;
    mDynamicCastGet(Geometry::BinIDSurface*,surface,ge);
    return surface ? new Geometry::BinIDElementEditor( *surface ) : nullptr;
}

} // namespace MPE
