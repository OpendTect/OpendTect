#ifndef uibatchlaunch_h
#define uibatchlaunch_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra/Bert
 Date:          Jan 2002/Mar 2014
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
#include "bufstringset.h"

class uiLabel;
class uiGenInput;
class uiListBox;
class uiBatchJobDispatcherSel;

mExpClass(uiIo) uiProcSettings : public uiDialog
{
public:
			uiProcSettings(uiParent*);
protected:

    bool		acceptOK(CallBacker*);

    uiGenInput*		nrinlfld_;
    uiGenInput*		clusterfld_;
};


mExpClass(uiIo) uiStartBatchJobDialog : public uiDialog
{
public:

				uiStartBatchJobDialog(uiParent*);

protected:

    BufferStringSet		filenames_;
    bool			canresume_;

    uiListBox*			jobsfld_;
    uiBatchJobDispatcherSel*	batchfld_;
    uiGenInput*			resumefld_;
    uiLabel*			invalidsellbl_;

    void			fillList(CallBacker*);
    void			itmSel(CallBacker*);
    void			launcherSel(CallBacker*);
    bool			acceptOK(CallBacker*);

    bool			canRun() const;

};


#endif
