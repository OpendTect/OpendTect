#ifndef polygonsurfaceedit_h
#define polygonsurfaceedit_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Y.C. Liu
 Date:          August 2008
 Contents:      Ranges
 RCS:           $Id: polygonsurfaceedit.h,v 1.3 2008-12-25 11:55:38 cvsranojay Exp $
________________________________________________________________________

-*/

#include "geeditor.h"

namespace Geometry
{
class PolygonSurface;

mClass PolygonSurfEditor : public ElementEditor
{
public:
    		PolygonSurfEditor( Geometry::PolygonSurface& );
    		~PolygonSurfEditor();

    bool 	mayTranslate2D( GeomPosID gpid ) const;
    Coord3	translation2DNormal( GeomPosID gpid ) const;   

protected:
    
   void		addedKnots(CallBacker*);    
};

};

#endif

