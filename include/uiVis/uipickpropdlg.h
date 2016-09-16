#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uivismarkerstyledlg.h"

class uiCheckBox;
class uiGenInput;

namespace Pick { class Set; };
namespace visSurvey { class PickSetDisplay; };


mExpClass(uiVis) uiPickPropDlg : public uiVisMarkerStyleDlg
{ mODTextTranslationClass(uiPickPropDlg);
public:
				uiPickPropDlg(uiParent* p,
					      Pick::Set& set,
					      visSurvey::PickSetDisplay* psd);
				~uiPickPropDlg();

protected:

    void			doFinalise(CallBacker*);
    void			sizeChg(CallBacker*);
    void			typeSel(CallBacker*);
    void			colSel(CallBacker*);
    bool			acceptOK();
    void			drawStyleCB(CallBacker*);
    void			drawSel(CallBacker*);

    uiGenInput*	drawstylefld_;
    uiCheckBox*	usedrawstylefld_;

    Pick::Set&			set_;
    visSurvey::PickSetDisplay*	psd_;

};
