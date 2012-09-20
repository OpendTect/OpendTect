#ifndef uigmtinfodlg_h
#define uigmtinfodlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Sep 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uigmtmod.h"
#include "uidialog.h"

class uiCheckBox;
class uiFileInput;
class uiLabel;

mClass(uiGMT) uiGMTInfoDlg : public uiDialog
{
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

#endif

