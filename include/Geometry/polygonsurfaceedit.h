#ifndef polygonsurfaceedit_h
#define polygonsurfaceedit_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Y.C. Liu
 Date:          August 2008
 Contents:      Ranges
 RCS:           $Id: polygonsurfaceedit.h,v 1.5 2012-08-03 13:00:28 cvskris Exp $
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "geeditor.h"

namespace Geometry
{
class PolygonSurface;

mClass(Geometry) PolygonSurfEditor : public ElementEditor
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


