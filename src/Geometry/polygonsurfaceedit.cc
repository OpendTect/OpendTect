/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "polygonsurfaceedit.h"

#include "polygonsurface.h"

namespace Geometry
{


PolygonSurfEditor::PolygonSurfEditor( Geometry::PolygonSurface& plg )
    : ElementEditor( plg )
{
    plg.nrpositionnotifier.notify( mCB(this,PolygonSurfEditor,addedKnots) );
}


PolygonSurfEditor::~PolygonSurfEditor()
{
    PolygonSurface& plg = reinterpret_cast<PolygonSurface&>(element);
    plg.nrpositionnotifier.remove( mCB(this,PolygonSurfEditor,addedKnots) );
}


bool PolygonSurfEditor::mayTranslate2D( GeomPosID gpid ) const
{ return translation2DNormal( gpid ).isDefined(); }


Coord3 PolygonSurfEditor::translation2DNormal( GeomPosID gpid ) const
{
    const PolygonSurface& plg =
			reinterpret_cast<const PolygonSurface&>( element );
    const int plgnr = RowCol::fromInt64(gpid).row();
    return plg.getPolygonNormal( plgnr );
}

void PolygonSurfEditor::addedKnots(CallBacker*)
{ editpositionchange.trigger(); }



} // namespace Geometry
