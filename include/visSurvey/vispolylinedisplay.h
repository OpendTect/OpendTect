#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
