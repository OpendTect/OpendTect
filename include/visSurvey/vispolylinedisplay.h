#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          November 2005
________________________________________________________________________

-*/

#include "vissurveycommon.h"
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
{ mODTextTranslationClass(PolyLineDisplay);
public:
				PolyLineDisplay();
				mDefaultFactoryInstantiation(
				    SurveyObject,PolyLineDisplay,
				    "PolyLineDisplay",
				    toUiString(sFactoryKeyword()));
    virtual const char*		getClassName() const
				{ return sFactoryKeyword(); }

    void			fillPolyLine(const TypeSet<Coord>&);
    void                        fillPolyLine(const Coord3&);
    void                        setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    const uiString&		errMsg() const { return errmsg_; }
    virtual			void setPixelDensity(float);

protected:

protected:
				~PolyLineDisplay();
    visBase::PolyLine*		polyline_;
};

};//namespace
