#ifndef vispolylinedisplay_h
#define vispolylinedisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          November 2005
 RCS:           $Id: vispolylinedisplay.h,v 1.5 2010-02-22 22:42:40 cvskris Exp $
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

    const char*			errMsg() const { return errmsg_.buf(); }

protected:

    visBase::PolyLine*			polyline_;
};
    
};//namespace

#endif
