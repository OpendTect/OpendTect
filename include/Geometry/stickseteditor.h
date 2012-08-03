#ifndef stickseteditor_h
#define stickseteditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          November 2008
 Contents:      Ranges
 RCS:           $Id: stickseteditor.h,v 1.4 2012-08-03 13:00:28 cvskris Exp $
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "geeditor.h"

namespace Geometry
{
class FaultStickSet;

mClass(Geometry) StickSetEditor : public ElementEditor
{
public:
    		StickSetEditor( Geometry::FaultStickSet& );
    		~StickSetEditor();

    bool	mayTranslate2D( GeomPosID ) const;
    Coord3	translation2DNormal( GeomPosID ) const;

protected:
    void	addedKnots(CallBacker*);
};

};

#endif


