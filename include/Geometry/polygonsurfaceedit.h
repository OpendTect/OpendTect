#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		August 2008
 Contents:	Ranges
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "geeditor.h"

namespace Geometry
{
class PolygonSurface;

mExpClass(Geometry) PolygonSurfEditor : public ElementEditor
{
public:
		PolygonSurfEditor( Geometry::PolygonSurface& );
		~PolygonSurfEditor();

    bool	mayTranslate2D( GeomPosID gpid ) const override;
    Coord3	translation2DNormal( GeomPosID gpid ) const override;

protected:
    
   void		addedKnots(CallBacker*);    
};

} // namespace Geometry
