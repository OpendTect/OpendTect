#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Asif
 * DATE     : Mar 2018
-*/

#include "vissurvobj.h"
#include "uitutmod.h"
#include "vistext.h"

namespace visSurvey
{

mExpClass(uiTut) TutorialDisplay: public visBase::VisualObjectImpl,
				  public visSurvey::SurveyObject
{ mODTextTranslationClass(TutorialDisplay)  /* to define tr(...) */
public:

    mDefaultFactoryInstantiation(visSurvey::SurveyObject,TutorialDisplay,
				"TutorialDisplay",
				toUiString(sFactoryKeyword()) )
			    /* to provide create() to instantiate the class */

				TutorialDisplay();
				TutorialDisplay(const uiString& texttodisplay,
					       const Coord3& position);
				~TutorialDisplay();

    const mVisTrans*		getDisplayTransformation() const;
    void			setDisplayTransformation(const mVisTrans*);

private:

    visBase::Text2*             text_;
    const mVisTrans*		transformation_;

    virtual const char*		getClassName() const
				{ return sFactoryKeyword(); }
				/* defined in mDefaultFactoryInstantiation */
};

}
