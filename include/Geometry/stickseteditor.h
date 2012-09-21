#ifndef stickseteditor_h
#define stickseteditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          November 2008
 Contents:      Ranges
 RCS:           $Id$
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


