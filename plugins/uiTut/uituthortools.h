#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R.K. Singh / Karthika
 * DATE     : Mar 2007
-*/

#include "uitutmod.h"
#include "uidialog.h"
#include "uilistbox.h"

class uiIOObjSel;
class uiGenInput;

mExpClass(uiTut) uiTutHorTools : public uiDialog
{ mODTextTranslationClass(uiTutHorTools);
public:

    			uiTutHorTools(uiParent*);
			~uiTutHorTools();

protected:

    void		choiceSel(CallBacker*);
    bool		acceptOK();

    bool		checkAttribName() const;
    bool		doThicknessCalc();
    bool		doSmoother();

    uiGenInput*		taskfld_;
    uiIOObjSel*		inpfld_;
    uiIOObjSel*		inpfld2_;
    uiGenInput*		selfld_;
    uiGenInput*		attribnamefld_;
    uiIOObjSel*		outfld_;
    uiGenInput*		strengthfld_;
};
