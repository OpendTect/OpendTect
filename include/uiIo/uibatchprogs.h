#ifndef uibatchprogs_h
#define uibatchprogs_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Apr 2003
 RCS:           $Id$
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
    bool		acceptOK(CallBacker*);
    void		filenmUpd(CallBacker*);
};


#endif


