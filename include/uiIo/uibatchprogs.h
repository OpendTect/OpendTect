#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Apr 2003
________________________________________________________________________

-*/


#include "uiiocommon.h"
#include "uidialog.h"

class uiComboBox;
class uiGroup;
class uiPushButton;
class uiTextEdit;
class uiTextFileDlg;
class BatchProgInfoList;


mExpClass(uiIo) uiBatchProgLaunch : public uiDialog
{ mODTextTranslationClass(uiBatchProgLaunch)
public:

			uiBatchProgLaunch(uiParent*);
			~uiBatchProgLaunch();

protected:

    uiComboBox*		progfld_;
    uiTextEdit*		commfld_;
    uiTextFileDlg*	browser_;
    uiPushButton*	exbut_;
    ObjectSet< ObjectSet<uiGroup> > inps_;

    BatchProgInfoList&	pil_;

    void		progSel(CallBacker*);
    void		exButPush(CallBacker*);
    bool		acceptOK();
    void		filenmUpd(CallBacker*);
};
