#ifndef uibatchlaunch_h
#define uibatchlaunch_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          Jan 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

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

    bool			acceptOK(CallBacker*);

    uiListBox*			jobsfld_;
    uiBatchJobDispatcherSel*	batchfld_;
    uiGenInput*			resumefld_;

};


#endif
