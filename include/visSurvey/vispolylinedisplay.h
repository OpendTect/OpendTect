#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          November 2005
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "vistransform.h"

namespace visBase { class PolyLine; }

namespace visSurvey
{

/*!\brief Used for displaying a polyline, preview for a random line created
  throught well path*/

mExpClass(visSurvey) PolyLineDisplay : public visBase::VisualObjectImpl,
				    public visSurvey::SurveyObject
{ mODTextTranslationClass(PolyLineDisplay);
public:
				PolyLineDisplay();
				mDefaultFactoryInstantiation(
				    visSurvey::SurveyObject,PolyLineDisplay,
				    "PolyLineDisplay",
				    toUiString(sFactoryKeyword()));

    void			fillPolyLine(const TypeSet<Coord>&);
    void			fillPolyLine(const Coord3&);
    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    const char*			errMsg() const override
				{ return errmsg_.str(); }
    void			setPixelDensity(float) override;

protected:

protected:
				~PolyLineDisplay();
    visBase::PolyLine*		polyline_;
};

} // namespace visSurvey
