#ifndef uibatchprogs_h
#define uibatchprogs_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Apr 2003
 RCS:           $Id: uibatchprogs.h,v 1.8 2009-01-08 07:23:07 cvsranojay Exp $
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
