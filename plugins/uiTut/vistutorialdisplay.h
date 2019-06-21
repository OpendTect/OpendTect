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
    class Text;
}


namespace visSurvey
{
mExpClass(uiTut) TutorialWellDisplay : public visBase::VisualObjectImpl,
					public visSurvey::SurveyObject
{
public:
				TutorialWellDisplay();
				~TutorialWellDisplay();

    void			loadAndDisplayWell(const DBKey&);

protected:

    visBase::PolyLine*		welltrack_;
    visBase::MarkerSet*		wellmarkers_;
    visBase::Text*		welllabels_;

    void			setDisplayTransformation(const mVisTrans*);
    void			displayWellLabel(visBase::Text*,
					    const uiString&,const Coord3&);

    virtual const char*		getClassName() const
				{ return typeid(*this).name(); }

};
} // namespace visSurvey
