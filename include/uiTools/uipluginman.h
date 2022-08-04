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

class uiCheckBox;
class uiGenInput;
class uiTextBrowser;
class uiTreeView;


/*!\brief Shows loaded plugins and allows adding */

mExpClass(uiTools) uiPluginMan : public uiDialog
{  mODTextTranslationClass(uiPluginMan)
public:
			uiPluginMan(uiParent*);

protected:

    uiTreeView*		pluginview_;

    uiGenInput*		namefld_;
    uiGenInput*		productfld_;
    uiGenInput*		creatorfld_;
    uiGenInput*		filenmfld_;
    uiGenInput*		versionfld_;
    uiTextBrowser*	infofld_;
    uiTextBrowser*	licensefld_;

    bool		rejectOK(CallBacker*) override;
    void		fillList();
    void		emptyFields();
    void		activateCB(CallBacker*);
    void		selChg(CallBacker*);
    void		loadPush(CallBacker*);

};


