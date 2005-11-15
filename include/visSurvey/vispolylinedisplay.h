#ifndef vispolylinedisplay_h
#define vispolylinedisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          November 2005
 RCS:           $Id: vispolylinedisplay.h,v 1.1 2005-11-15 16:16:56 cvshelene Exp $
________________________________________________________________________

-*/

#include "visobject.h"
#include "vissurvobj.h"
#include "vistransform.h"

class Coord;

namespace visBase { class PolyLine; }


/*!\brief Used for displaying a polyline, preview for a random line created 
  throught well path*/

namespace visSurvey
{

class PolyLineDisplay :     public visBase::VisualObjectImpl,
                            public visSurvey::SurveyObject
{
public:
    static PolyLineDisplay*	create()
    				mCreateDataObj(PolyLineDisplay);
    				~PolyLineDisplay();

    void			fillPolyLine(const TypeSet<Coord>&);
    void                        setDisplayTransformation(mVisTrans*);
    mVisTrans*                  getDisplayTransformation();

protected:

    visBase::PolyLine*			polyline_;
};
    
};//namespace

#endif
