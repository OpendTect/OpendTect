#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Oct 2003
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
class uiListBox;
class uiTextEdit;
class uiCheckBox;


/*!\brief Shows loaded plugins and allows adding */

mExpClass(uiTools) uiPluginMan : public uiDialog
{  mODTextTranslationClass(uiPluginMan);
public:
			uiPluginMan(uiParent*);

protected:

    uiListBox*		listfld;
    uiTextEdit*		infofld;
    uiCheckBox*		selatstartfld;

    bool		rejectOK();
    void		fillList();
    void		selChg(CallBacker*);
    void		loadPush(CallBacker*);

};
