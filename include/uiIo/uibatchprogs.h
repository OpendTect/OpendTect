#ifndef uibatchprogs_h
#define uibatchprogs_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          Apr 2003
 RCS:           $Id: uibatchprogs.h,v 1.3 2003-05-02 14:44:17 arend Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class uiGenInput;
class uiTextEdit;
class uiPushButton;
class uiFileBrowser;
class BatchProgInfoList;
class uiLabeledComboBox;



class uiBatchProgLaunch : public uiDialog
{
public:

			uiBatchProgLaunch(uiParent*);
			~uiBatchProgLaunch();

protected:

    uiLabeledComboBox*	progfld;
    uiTextEdit*		commfld;
    uiFileBrowser*	browser;
    uiPushButton*	exbut;
    ObjectSet< ObjectSet<uiGenInput> > inps;

    BatchProgInfoList&	pil;

    void		progSel(CallBacker*);
    void		exButPush(CallBacker*);
    bool		acceptOK(CallBacker*);
    void		filenmUpd(CallBacker*);
};


#endif
