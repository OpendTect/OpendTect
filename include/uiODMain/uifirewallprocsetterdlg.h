#pragma once

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : May 2019
-*/

#include "uiodmainmod.h"
#include "uidialog.h"

#include "enums.h"
#include "procdescdata.h"

class uiButton;
class uiGenInput;
class uiListBox;


mExpClass(uiODMain) uiFirewallProcSetter : public uiDialog
{ mODTextTranslationClass(uiFireWallProcSetter)
public:
				uiFirewallProcSetter(uiParent*,
					ProcDesc::DataEntry::ActionType,
					const BufferString* path=nullptr,
					const BufferString* pypath=nullptr);
				~uiFirewallProcSetter();

    bool			hasWorkToDo() const;

protected:

    uiListBox*			odproclistbox_;
    uiListBox*			pythonproclistbox_;
    uiGenInput*			addremfld_		= nullptr;

    BufferString		getPythonInstDir();
    uiStringSet			getPythonExecList();
    BufferStringSet		getProcList(ProcDesc::DataEntry::Type);
    void			init();
    void			setEmpty();

    bool			acceptOK(CallBacker*);
    void			statusUpdateODProcCB(CallBacker*);
    void			statusUpdatePyProcCB(CallBacker*);
    void			selectionChgCB(CallBacker*);
    void			updateCB(CallBacker*);

    bool			toadd_;
    BufferString		exepath_;
    BufferStringSet		odprocnms_;
    uiStringSet			odprocdescs_;
    BufferStringSet		pyprocnms_;
    uiStringSet			pyprocdescs_;
    BufferString		pypath_;
    BufferString		acttypstr_;
};
