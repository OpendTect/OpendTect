#ifndef uigmtinfodlg_h
#define uigmtinfodlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Sep 2010
 RCS:           $Id: uigmtinfodlg.h,v 1.1 2010/09/15 12:06:09 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiCheckBox;
class uiFileInput;
class uiLabel;

mClass uiGMTInfoDlg : public uiDialog
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
