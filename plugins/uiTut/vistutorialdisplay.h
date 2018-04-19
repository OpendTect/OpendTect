#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Asif
 * DATE     : Mar 2018
-*/

#include "vissurvobj.h"
#include "visobject.h"
#include "uitutmod.h"
#include "dbdir.h"

namespace visBase /*forward declaration*/
{
    class PolyLine;
    class Text2;
}

namespace visSurvey
{
    struct TutorialWellDisplay:public visBase::VisualObjectImpl,
				  public visSurvey::SurveyObject
    {
	visBase::PolyLine*	welltrack_;
	visBase::MarkerSet*	wellmarkers_;
	visBase::Text2*         welllabels_;
	DBKey                   key_;

	void			setDisplayTransformation(const mVisTrans*);

				~TutorialWellDisplay();
				TutorialWellDisplay(const DBKey);
				TutorialWellDisplay();
	
	virtual const char*	getClassName() const 
					       { return typeid(*this).name(); }
	

	void			displayWellLabel(visBase::Text2*, 
						const uiString&,const Coord3&);
	void			loadAndDisplayWell();
    };
} // namespace visSurvey
