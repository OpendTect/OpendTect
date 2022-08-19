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
