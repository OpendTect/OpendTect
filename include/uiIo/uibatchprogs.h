#ifndef uibatchprogs_h
#define uibatchprogs_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          Apr 2003
 RCS:           $Id: uibatchprogs.h,v 1.1 2003-04-25 14:03:47 bert Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
class uiGenInput;
class uiTextEdit;
class BatchProgInfoList;
class uiLabeledComboBox;



class uiBatchProgLaunch : public uiDialog
{
public:

			uiBatchProgLaunch(uiParent*);

protected:

    uiLabeledComboBox*	progfld;
    uiTextEdit*		commfld;
    ObjectSet< ObjectSet<uiGenInput> > inps;

    BatchProgInfoList&	pil;

    void		progSel(CallBacker*);
    bool		acceptOK(CallBacker*);
};


#endif
