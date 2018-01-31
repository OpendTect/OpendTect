#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"

class uiCheckBox;
class uiGenInput;
class uiMarkerStyle3D;

namespace Pick { class Set; }
namespace visSurvey { class PickSetDisplay; }


mExpClass(uiVis) uiPickPropDlg : public uiDialog
{ mODTextTranslationClass(uiPickPropDlg)
public:
				uiPickPropDlg(uiParent* p,
					      Pick::Set& set,
					      visSurvey::PickSetDisplay* psd);
				~uiPickPropDlg();

protected:

    bool			acceptOK();
    void			styleSel(CallBacker*);
    void			drawStyleCB(CallBacker*);
    void			drawSel(CallBacker*);
    void			useThresholdCB(CallBacker*);
    void			thresholdChangeCB(CallBacker*);

    uiCheckBox*			usedrawstylefld_;
    uiGenInput*			drawstylefld_;
    uiMarkerStyle3D*		stylefld_;
    uiCheckBox*			usethresholdfld_;
    uiGenInput*			thresholdfld_;

    Pick::Set&			set_;
    visSurvey::PickSetDisplay*	psd_;

};
