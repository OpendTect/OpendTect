#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
				uiFirewallProcSetter(uiParent*);
				~uiFirewallProcSetter();
    mDeprecated("Use the function from ProcDesc::DataEntry::hasWorkToDo")
    bool			hasWorkToDo() const;
    void			updateUI(const BufferString& path,
				    const BufferString& pypath,
				    ProcDesc::DataEntry::ActionType acttyp =
					    ProcDesc::DataEntry::AddNRemove);
protected:

    uiListBox*			odproclistbox_;
    uiListBox*			pythonproclistbox_;
    uiGenInput*			addremfld_		= nullptr;

    BufferString		getPythonInstDir();
    uiStringSet			getPythonExecList();
    BufferStringSet		getProcList(ProcDesc::DataEntry::Type);
    void			init();
    void			initUI(const BufferString* path = nullptr,
				    const BufferString* pypath = nullptr,
				    ProcDesc::DataEntry::ActionType acttyp =
					    ProcDesc::DataEntry::AddNRemove);
    void			setEmpty();

    bool			acceptOK(CallBacker*);
    void			statusUpdateODProcCB(CallBacker*);
    void			statusUpdatePyProcCB(CallBacker*);
    void			selectionChgCB(CallBacker*);
    void			updateCB(CallBacker*);
    void			updateAddRemFld(CallBacker*);

    bool			toadd_;
    BufferString		exepath_;
    BufferStringSet		odprocnms_;
    uiStringSet			odprocdescs_;
    BufferStringSet		pyprocnms_;
    uiStringSet			pyprocdescs_;
    BufferString		pypath_;
    BufferString		acttypstr_;
    uiString			sStatusBarMsg()
						{ return tr("Path : %1"); }
};
