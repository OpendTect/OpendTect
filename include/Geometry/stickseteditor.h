#ifndef stickseteditor_h
#define stickseteditor_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        J.C. Glas
 Date:          November 2008
 Contents:      Ranges
 RCS:           $Id: stickseteditor.h,v 1.1 2008-11-18 13:28:53 cvsjaap Exp $
________________________________________________________________________

-*/

#include "geeditor.h"

namespace Geometry
{
class FaultStickSet;

class StickSetEditor : public ElementEditor
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

