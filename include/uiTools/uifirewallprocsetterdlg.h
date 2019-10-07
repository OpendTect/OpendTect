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
    BufferStringSet	    getPythonExecList();

    ActionType		    acttyp_;
    BufferString	    exepath_;
};
#endif
