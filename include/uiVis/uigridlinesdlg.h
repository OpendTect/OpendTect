#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivismod.h"
#include "uidialog.h"
#include "ranges.h"

class uiCheckBox;
class uiGenInput;
class uiSelLineStyle;

namespace visSurvey { class PlaneDataDisplay; }

mExpClass(uiVis) uiGridLinesDlg : public uiDialog
{ mODTextTranslationClass(uiGridLinesDlg);
public:
			uiGridLinesDlg(uiParent*,visSurvey::PlaneDataDisplay*);

protected:

    void		setParameters();
    void 		showGridLineCB(CallBacker*);
    bool                acceptOK(CallBacker*);

    uiCheckBox*		inlfld_;
    uiCheckBox*		crlfld_;
    uiCheckBox*		zfld_;
    uiGenInput*		inlspacingfld_;
    uiGenInput*		crlspacingfld_;
    uiGenInput*		zspacingfld_;
    uiSelLineStyle*     lsfld_;
    uiCheckBox*		applyallfld_;

    visSurvey::PlaneDataDisplay*	pdd_;
};
