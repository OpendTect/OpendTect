#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitutmod.h"
#include "uidialog.h"

class uiIOObjSel;
class uiGenInput;

mExpClass(uiTut) uiTutHorTools : public uiDialog
{ mODTextTranslationClass(uiTutHorTools);
public:

    			uiTutHorTools(uiParent*);
			~uiTutHorTools();

protected:

    void		choiceSel(CallBacker*);
    bool		acceptOK(CallBacker*);

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
