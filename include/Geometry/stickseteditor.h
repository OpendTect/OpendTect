#ifndef stickseteditor_h
#define stickseteditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          November 2008
 Contents:      Ranges
 RCS:           $Id: stickseteditor.h,v 1.3 2009/07/22 16:01:16 cvsbert Exp $
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

