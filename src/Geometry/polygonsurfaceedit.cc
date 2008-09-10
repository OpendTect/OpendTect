/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : J.C.Glas
 * DATE     : Feburary 2008
___________________________________________________________________

-*/

static const char* rcsID = "$Id: polygonsurfaceedit.cc,v 1.1 2008-09-10 13:00:08 cvsyuancheng Exp $";

#include "polygonsurfaceedit.h"

#include "polygonsurface.h"

namespace Geometry 
{


PolygonSurfEditor::PolygonSurfEditor( Geometry::PolygonSurface& plg )
    : ElementEditor( plg )
{
}


PolygonSurfEditor::~PolygonSurfEditor()
{
    PolygonSurface& plg = reinterpret_cast<PolygonSurface&>(element);
}


bool PolygonSurfEditor::mayTranslate3D( GeomPosID gpid ) const
{ return translation2DNormal( gpid ).isDefined(); }


Coord3 PolygonSurfEditor::translation2DNormal( GeomPosID gpid ) const
{
    const PolygonSurface& plg = 
			reinterpret_cast<const PolygonSurface&>( element );
    const int plgnr = RowCol(gpid).r();
    return plg.getPolygonNormal( plgnr );
}


    
}; //Namespace
