#ifndef uifirewallprocsetterdlg_h
#define uifirewallprocsetterdlg_h

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : May 2019
-*/

#include "uitoolsmod.h"
#include "uidialog.h"

#include "enums.h"
#include "procdescdata.h"


class uiListBox;
class uiButton;
class uiGenInput;

mExpClass(uiTools) uiFirewallProcSetter : public uiDialog
{ mODTextTranslationClass(uiFireWallProcSetter)
public:
    enum ActionType	    { Add, Remove, AddNRemove };
			    mDeclareEnumUtils(ActionType)

			    uiFirewallProcSetter(uiParent*,ActionType,
				const BufferString&path=BufferString::empty());
			    ~uiFirewallProcSetter();
protected:

    uiListBox*		    odproclistbox_;
    uiListBox*		    pythonproclistbox_;
    uiListBox*		    deeplearninglistbox_;
    uiGenInput*		    addremfld_;

    bool		    acceptOK(CallBacker*);
    BufferString	    getPythonInstDir();
    uiStringSet		    getPythonExecList();

    BufferStringSet	    getSelProcList(ProcDesc::DataEntry::Type);

    ActionType		    acttyp_;
    BufferString	    exepath_;

    BufferStringSet	    odprocnms_;
    uiStringSet		    odprocdescs_;
    BufferStringSet	    pyprocnms_;
    uiStringSet		    pyprocdescs_;
};
#endif
