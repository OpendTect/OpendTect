#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2006
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
    bool			acceptOK();
    bool			rejectOK();
    void			changeCB(CallBacker*);

    unsigned char		initaltrans_;
    int				attrib_;
    visSurvey::SurveyObject&	so_;
    uiSlider*			slider_;
};
