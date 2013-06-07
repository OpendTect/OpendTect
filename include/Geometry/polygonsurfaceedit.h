#ifndef polygonsurfaceedit_h
#define polygonsurfaceedit_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Y.C. Liu
 Date:          August 2008
 Contents:      Ranges
 RCS:           $Id$
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

