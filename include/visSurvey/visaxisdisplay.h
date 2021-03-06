#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		Feb 2020
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
