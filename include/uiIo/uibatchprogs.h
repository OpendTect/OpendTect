#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class uiComboBox;
class uiGenInput;
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
    ObjectSet< ObjectSet<uiGenInput> > inps_;

    BatchProgInfoList&	pil_;

    void		progSel(CallBacker*);
    void		exButPush(CallBacker*);
    bool		acceptOK(CallBacker*) override;
    void		filenmUpd(CallBacker*);
};
