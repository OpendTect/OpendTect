/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2005
________________________________________________________________________

-*/

#include "horizoneditor.h"

#include "geeditorimpl.h"
#include "binidsurface.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "mpeengine.h"


namespace MPE
{

HorizonEditor::HorizonEditor( EM::Horizon3D& hor3d )
    : ObjectEditor(hor3d)
{
}


ObjectEditor* HorizonEditor::create( EM::EMObject& emobj )
{
    mDynamicCastGet(EM::Horizon3D*,horizon,&emobj);
    return horizon ? new HorizonEditor( *horizon ) : 0;
}


void HorizonEditor::initClass()
{ MPE::EditorFactory().addCreator( create, EM::Horizon3D::typeStr() ); }


Geometry::ElementEditor* HorizonEditor::createEditor()
{
    const Geometry::Element* ge = emObject().geometryElement();
    if ( !ge ) return 0;

    mDynamicCastGet(const Geometry::BinIDSurface*,surface,ge);
    if ( !surface ) return 0;

    return new Geometry::BinIDElementEditor(
		*const_cast<Geometry::BinIDSurface*>(surface) );
}


void HorizonEditor::getEditIDs( TypeSet<EM::PosID>& ids ) const
{
    ids.erase();
}



Horizon2DEditor::Horizon2DEditor( EM::Horizon2D& hor2d )
    : ObjectEditor(hor2d)
{}

ObjectEditor* Horizon2DEditor::create( EM::EMObject& emobj )
{
    mDynamicCastGet(EM::Horizon2D*,hor2d,&emobj);
    return hor2d ? new Horizon2DEditor( *hor2d ) : 0;
}


void Horizon2DEditor::initClass()
{ MPE::EditorFactory().addCreator( create, EM::Horizon2D::typeStr() ); }


Geometry::ElementEditor* Horizon2DEditor::createEditor()
{
    const Geometry::Element* ge = emObject().geometryElement();
    if ( !ge ) return 0;

    mDynamicCastGet(const Geometry::BinIDSurface*,surface,ge);
    if ( !surface ) return 0;

    return new Geometry::BinIDElementEditor(
		*const_cast<Geometry::BinIDSurface*>(surface) );
}

} // namespace MPE
