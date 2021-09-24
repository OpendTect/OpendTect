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


class uiListBox;
class uiButton;
class uiGenInput;



typedef ProcDesc::DataEntry PDE;

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
				    PDE::ActionType acttyp = PDE::AddNRemove);
protected:

    uiListBox*			odproclistbox_;
    uiListBox*			pythonproclistbox_;
    uiGenInput*			addremfld_;

    BufferString		getPythonInstDir();
    uiStringSet			getPythonExecList();
    BufferStringSet		getProcList(ProcDesc::DataEntry::Type);
    void			init();
    void			initUI(const BufferString* path = nullptr,
				    const BufferString* pypath = nullptr,
				    PDE::ActionType acttyp = PDE::AddNRemove);
    void			setEmpty();

    bool			acceptOK(CallBacker*);
    void			statusUpdateODProcCB(CallBacker*);
    void			statusUpdatePyProcCB(CallBacker*);
    void			selectionChgCB(CallBacker*);
    void			updateCB(CallBacker*);
    void			updateAddRemFld(CallBacker*);

    bool			toadd_;
    BufferString		exepath_;
    BufferStringSet		odv6procnms_;
    BufferStringSet		odv7procnms_;
    uiStringSet			odprocdescs_;
    BufferStringSet		pyprocnms_;
    uiStringSet			pyprocdescs_;
    BufferString		pypath_;

    uiString			sStatusBarMsg()
				{ return tr("Path : %1"); }
};

