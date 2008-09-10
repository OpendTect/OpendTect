#ifndef polygonsurfaceedit_h
#define polygonsurfaceedit_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Y.C. Liu
 Date:          August 2008
 Contents:      Ranges
 RCS:           $Id: polygonsurfaceedit.h,v 1.1 2008-09-10 13:00:08 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "geeditor.h"

namespace Geometry
{
class PolygonSurface;

class PolygonSurfEditor : public ElementEditor
{
public:
    		PolygonSurfEditor( Geometry::PolygonSurface& );
    		~PolygonSurfEditor();
    bool 	mayTranslate3D( GeomPosID gpid ) const;
    Coord3	translation2DNormal( GeomPosID gpid ) const;    
};

};

#endif

