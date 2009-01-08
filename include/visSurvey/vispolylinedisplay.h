#ifndef vispolylinedisplay_h
#define vispolylinedisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          November 2005
 RCS:           $Id: vispolylinedisplay.h,v 1.3 2009-01-08 10:25:45 cvsranojay Exp $
________________________________________________________________________

-*/

#include "visobject.h"
#include "vissurvobj.h"
#include "vistransform.h"

class Coord;
class Coord3;

namespace visBase { class PolyLine; }


/*!\brief Used for displaying a polyline, preview for a random line created 
  throught well path*/

namespace visSurvey
{

mClass PolyLineDisplay :     public visBase::VisualObjectImpl,
                            public visSurvey::SurveyObject
{
public:
    static PolyLineDisplay*	create()
    				mCreateDataObj(PolyLineDisplay);
    				~PolyLineDisplay();

    void			fillPolyLine(const TypeSet<Coord>&);
    void                        fillPolyLine(const Coord3&);
    void                        setDisplayTransformation(mVisTrans*);
    mVisTrans*                  getDisplayTransformation();

protected:

    visBase::PolyLine*			polyline_;
};
    
};//namespace

#endif
