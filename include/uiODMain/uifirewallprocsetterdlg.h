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



mExpClass(uiODMain) uiFirewallProcSetter : public uiDialog
{ mODTextTranslationClass(uiFireWallProcSetter)
public:
   typedef ProcDesc::DataEntry PDE;


			    uiFirewallProcSetter(uiParent*,PDE::ActionType,
				const BufferString&path=BufferString::empty(),
			    const BufferString&pypath = BufferString::empty());
			    ~uiFirewallProcSetter();
protected:

    uiListBox*		    odproclistbox_;
    uiListBox*		    pythonproclistbox_;
    uiGenInput*		    addremfld_;

    BufferString	    getPythonInstDir();
    uiStringSet		    getPythonExecList();
    BufferStringSet	    getProcList(ProcDesc::DataEntry::Type);
    void		    init();
    void		    setEmpty();

    bool		    acceptOK(CallBacker*);
    void		    statusUpdateODProcCB(CallBacker*);
    void		    statusUpdatePyProcCB(CallBacker*);
    void		    selectionChgCB(CallBacker*);

    bool		    toadd_;
    BufferString	    exepath_;
    BufferStringSet	    odv6procnms_;
    BufferStringSet	    odv7procnms_;
    uiStringSet		    odprocdescs_;
    BufferStringSet	    pyprocnms_;
    uiStringSet		    pyprocdescs_;
    BufferString	    pypath_;

    uiString		    sStatusBarMsg()
			    { return tr("Selected process path : %1"); }
};

