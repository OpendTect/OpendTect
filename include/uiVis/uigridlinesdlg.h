#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          February 2006
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
    bool		acceptOK();

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
