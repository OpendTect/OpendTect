/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : J.C.Glas
 * DATE     : Feburary 2008
___________________________________________________________________

-*/

static const char* rcsID = "$Id: polygonsurfaceedit.cc,v 1.2 2008-09-10 13:35:09 cvsyuancheng Exp $";

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
    const int plgnr = RowCol(gpid).r();
    return plg.getPolygonNormal( plgnr );
}

void PolygonSurfEditor::addedKnots(CallBacker*)
{ editpositionchange.trigger(); }


    
}; //Namespace
