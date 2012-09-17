#ifndef uibatchprogs_h
#define uibatchprogs_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Apr 2003
 RCS:           $Id: uibatchprogs.h,v 1.9 2009/07/22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class uiGenInput;
class uiTextEdit;
class uiPushButton;
class uiTextFileDlg;
class BatchProgInfoList;
class uiLabeledComboBox;



mClass uiBatchProgLaunch : public uiDialog
{
public:

			uiBatchProgLaunch(uiParent*);
			~uiBatchProgLaunch();

protected:

    uiLabeledComboBox*	progfld;
    uiTextEdit*		commfld;
    uiTextFileDlg*	browser;
    uiPushButton*	exbut;
    ObjectSet< ObjectSet<uiGenInput> > inps;

    BatchProgInfoList&	pil;

    void		progSel(CallBacker*);
    void		exButPush(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		filenmUpd(CallBacker*);
};


#endif
