#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtmod.h"
#include "uidialog.h"

class uiCheckBox;
class uiFileInput;
class uiLabel;

mExpClass(uiGMT) uiGMTInfoDlg : public uiDialog
{ mODTextTranslationClass(uiGMTInfoDlg);
public:
			uiGMTInfoDlg(uiParent*);
protected:
    void		gmtPushCB(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		selCB(CallBacker*);

    uiCheckBox*		chkbut_;
    uiFileInput*	gmtpath_;
    uiLabel*		label_;
};
