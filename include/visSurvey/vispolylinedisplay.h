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


namespace visBase { class PolyLine; }


/*!\brief Used for displaying a polyline, preview for a random line created
  throught well path*/

namespace visSurvey
{

mExpClass(visSurvey) PolyLineDisplay : public visBase::VisualObjectImpl,
				    public visSurvey::SurveyObject
{
public:
				PolyLineDisplay();
				mDefaultFactoryInstantiation( 
				    visSurvey::SurveyObject,PolyLineDisplay,
				    "PolyLineDisplay", sFactoryKeyword() );

    void			fillPolyLine(const TypeSet<Coord>&);
    void                        fillPolyLine(const Coord3&);
    void                        setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    const char*			errMsg() const { return errmsg_.str(); }
    virtual			void setPixelDensity(float);

protected:

protected:
				~PolyLineDisplay();
    visBase::PolyLine*		polyline_;
};
    
};//namespace

#endif

