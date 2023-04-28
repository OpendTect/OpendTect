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
namespace visBase { class TopBotImage; }


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

    void			newImage(bool istop,const MultiID&);
    void			setOn(bool istop,bool yn);
    void			setZ(bool istop,float z);
    void			setCoord(bool,const Coord3&, const Coord3&);
    void			setTransparency(bool istop,float tp);
    void			newFile(bool,const char*);
    visBase::TopBotImage*	getImage(bool istop);

    friend class	uiSurvTopBotImageGrp;

};
