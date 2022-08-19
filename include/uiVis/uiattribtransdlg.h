#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"

namespace visSurvey { class SurveyObject; }
class uiSlider;

mExpClass(uiVis) uiAttribTransDlg : public uiDialog
{ mODTextTranslationClass(uiAttribTransDlg);
public:
				uiAttribTransDlg(uiParent*,
						 visSurvey::SurveyObject&,
						 int attrib);
protected:
    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			changeCB(CallBacker*);

    unsigned char		initaltrans_;
    int				attrib_;
    visSurvey::SurveyObject&	so_;
    uiSlider*			slider_;
};
