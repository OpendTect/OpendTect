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


// HorizonEditor

RefMan<MPE::HorizonEditor>
			MPE::HorizonEditor::create( EM::Horizon3D& hor )
{
    return new HorizonEditor( hor );
}


MPE::HorizonEditor::HorizonEditor( const EM::Horizon3D& hor3d )
    : ObjectEditor(hor3d)
{
}


MPE::HorizonEditor::~HorizonEditor()
{}


Geometry::ElementEditor* MPE::HorizonEditor::createEditor()
{
    RefMan<EM::EMObject> emobject = emObject();
    Geometry::Element* ge = emobject ? emobject->geometryElement() : nullptr;
    mDynamicCastGet(Geometry::BinIDSurface*,surface,ge);
    return surface ? new Geometry::BinIDElementEditor( *surface ) : nullptr;
}


void MPE::HorizonEditor::getEditIDs( TypeSet<EM::PosID>& ids ) const
{
    ids.erase();
}


// Horizon2DEditor

RefMan<MPE::Horizon2DEditor>
			MPE::Horizon2DEditor::create( EM::Horizon2D& hor )
{
    return new Horizon2DEditor( hor );
}


MPE::Horizon2DEditor::Horizon2DEditor( const EM::Horizon2D& hor2d )
    : ObjectEditor(hor2d)
{}


MPE::Horizon2DEditor::~Horizon2DEditor()
{}


Geometry::ElementEditor* MPE::Horizon2DEditor::createEditor()
{
    RefMan<EM::EMObject> emobject = emObject();
    Geometry::Element* ge = emobject ? emobject->geometryElement() : nullptr;
    mDynamicCastGet(Geometry::BinIDSurface*,surface,ge);
    return surface ? new Geometry::BinIDElementEditor( *surface ) : nullptr;
}
