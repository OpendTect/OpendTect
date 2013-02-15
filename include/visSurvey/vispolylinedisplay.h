#ifndef vispolylinedisplay_h
#define vispolylinedisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          November 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "vistransform.h"

class Coord;
class Coord3;

namespace visBase { class PolyLine; }

namespace visSurvey
{

/*!
\brief Used for displaying a visBase::PolyLine, preview for a random line
created throughout well path.
*/

mExpClass(visSurvey) PolyLineDisplay : public visBase::VisualObjectImpl,
				    public visSurvey::SurveyObject
{
public:
    static PolyLineDisplay*	create()
    				mCreateDataObj(PolyLineDisplay);
    				~PolyLineDisplay();

    void			fillPolyLine(const TypeSet<Coord>&);
    void                        fillPolyLine(const Coord3&);
    void                        setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    const char*			errMsg() const { return errmsg_.str(); }

protected:

    visBase::PolyLine*		polyline_;
};
    
};//namespace

#endif

