#pragma once

/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Prajjaval Singh
 * DATE     : May 2019
-*/

#include "uitoolsmod.h"
#include "uidialog.h"

class uiListBox;
class uiButton;
class uiGenInput;

mExpClass(uiTools) uiFirewallProcSetter : public uiDialog
{ mODTextTranslationClass(uiFireWallProcSetter)
public:
			    uiFirewallProcSetter(uiParent*);
			    ~uiFirewallProcSetter();
protected:

    uiListBox*		    odproclistbox_;
    uiListBox*		    pythonproclistbox_;
    uiGenInput*		    addremfld_;

    bool		    acceptOK();
    BufferString	    getPythonInstDir();
    BufferStringSet	    getPythonExecList();
};
