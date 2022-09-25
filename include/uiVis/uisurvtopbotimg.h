#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			~uiSurvTopBotImageDlg();

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
