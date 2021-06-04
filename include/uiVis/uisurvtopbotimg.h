#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          February 2006
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"

class uiSurvTopBotImageGrp;
namespace visSurvey { class Scene; }


/*! sets the top and/or bottom images. */

mExpClass(uiVis) uiSurvTopBotImageDlg : public uiDialog
{ mODTextTranslationClass(uiSurvTopBotImageDlg);
public:
			uiSurvTopBotImageDlg(uiParent*,visSurvey::Scene*);

protected:

    visSurvey::Scene*		scene_;

    uiSurvTopBotImageGrp*	topfld_;
    uiSurvTopBotImageGrp*	botfld_;

    void		setOn(bool,bool);
    void		setCoord(bool,const Coord3&, const Coord3&);
    void		setTransparency(bool,float);
    void		newFile(bool,const char*);

    friend class	uiSurvTopBotImageGrp;

};

