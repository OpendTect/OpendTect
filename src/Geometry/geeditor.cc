/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "geeditor.h"

namespace Geometry
{


ElementEditor::ElementEditor( Geometry::Element& el )
    : element( el )
    , editpositionchange(this)
{ }


ElementEditor::~ElementEditor()
{ }


void ElementEditor::getEditIDs( TypeSet<GeomPosID>& ids ) const
{ element.getPosIDs( ids ); }


Coord3 ElementEditor::getPosition( GeomPosID gpid ) const
{ return element.getPosition( gpid ); }


bool ElementEditor::setPosition( GeomPosID gpid,  const Coord3& np )
{ return element.setPosition( gpid, np ); }


bool ElementEditor::mayTranslate1D( GeomPosID ) const	{ return false; }


Coord3 ElementEditor::translation1DDirection( GeomPosID ) const
{ return Coord3::udf(); }

bool ElementEditor::mayTranslate2D( GeomPosID ) const	{ return false; }


Coord3 ElementEditor::translation2DNormal( GeomPosID ) const
{ return Coord3::udf(); }


bool ElementEditor::mayTranslate3D( GeomPosID ) const	{ return false; }


bool ElementEditor::maySetNormal( GeomPosID ) const	{ return false; }


Coord3 ElementEditor::getNormal( GeomPosID ) const
{ return Coord3::udf(); }


bool ElementEditor::setNormal( GeomPosID, const Coord3& )
{ return true; }


bool ElementEditor::maySetDirection( GeomPosID ) const	{ return false; }


Coord3 ElementEditor::getDirectionPlaneNormal( GeomPosID ) const
{ return Coord3::udf(); }


Coord3 ElementEditor::getDirection( GeomPosID ) const
{ return Coord3::udf(); }


bool ElementEditor::setDirection( GeomPosID, const Coord3& )
{ return true; }



} // namespace Geometry
