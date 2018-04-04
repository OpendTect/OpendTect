#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Asif
 * DATE     : Mar 2018
-*/

#include "vissurvobj.h"
#include "visobject.h"
#include "uitutmod.h"

namespace visBase /*forward declaration*/
{
    class PolyLine;
    class Text2;
}

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
				~TutorialDisplay();

    void			displayText(const uiString& texttodisplay,
	    				    const Coord3& position);
    void			displayAllWells();

    const mVisTrans*		getDisplayTransformation() const;
    void			setDisplayTransformation(const mVisTrans*);

private:
    virtual const char*		getClassName() const
				{ return sFactoryKeyword(); }
				/* defined in mDefaultFactoryInstantiation */

    visBase::Text2*             text_;
    visBase::Text2*             welllabels_;
    const mVisTrans*		transformation_;

    struct WellDisplay
    {
	visBase::PolyLine*	welltrack_;
	visBase::MarkerSet*	wellmarkers_;

	void			removeFromNodeandUnref(TutorialDisplay*) const;
	void			setDisplayTransformation(const mVisTrans*);
    };
	
    ObjectSet<WellDisplay>	wells_;
    				/* One for each well */
	

    void			displayWell(const IOObj&);
    void			displayWellLabel(const uiString&,const Coord3&);
    void			removeFromNodeAndUnRef();
};
} // namespace visSurvey
