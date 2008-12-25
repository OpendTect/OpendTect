#ifndef stickseteditor_h
#define stickseteditor_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        J.C. Glas
 Date:          November 2008
 Contents:      Ranges
 RCS:           $Id: stickseteditor.h,v 1.2 2008-12-25 11:55:38 cvsranojay Exp $
________________________________________________________________________

-*/

#include "geeditor.h"

namespace Geometry
{
class FaultStickSet;

mClass StickSetEditor : public ElementEditor
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

