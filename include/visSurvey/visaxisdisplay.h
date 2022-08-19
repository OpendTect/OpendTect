#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"
#include "visaxes.h"
#include "vissurvobj.h"


namespace visSurvey
{

mExpClass(visSurvey) AxisDisplay : public visBase::Axes
				    , public visSurvey::SurveyObject
{ mODTextTranslationClass(AxisDisplay)
public:

				AxisDisplay();
				mDefaultFactoryInstantiation(
				    visSurvey::SurveyObject, AxisDisplay,
				    "AxisDisplay",
				    toUiString(sFactoryKeyword()))
};

} // namespace visSurvey
