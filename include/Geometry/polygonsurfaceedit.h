#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
