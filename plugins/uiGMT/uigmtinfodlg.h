#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Sep 2010
________________________________________________________________________

-*/

#include "uigmtmod.h"
#include "uidialog.h"

class uiCheckBox;
class uiFileSel;
class uiLabel;


mExpClass(uiGMT) uiGMTInfoDlg : public uiDialog
{ mODTextTranslationClass(uiGMTInfoDlg);
public:
			uiGMTInfoDlg(uiParent*);
protected:
    void		gmtPushCB(CallBacker*);
    bool		acceptOK();
    void		selCB(CallBacker*);

    uiCheckBox*		chkbut_;
    uiFileSel*		gmtpath_;
    uiLabel*		label_;
};
