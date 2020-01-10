#ifndef uipluginman_h
#define uipluginman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Oct 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

class uiCheckBox;
class uiGenInput;
class uiTextEdit;
class uiTreeView;


/*!\brief Shows loaded plugins and allows adding */

mExpClass(uiTools) uiPluginMan : public uiDialog
{  mODTextTranslationClass(uiPluginMan)
public:
			uiPluginMan(uiParent*);

protected:

    uiTreeView*		pluginview_;
    uiCheckBox*		selatstartfld_;

    uiGenInput*		namefld_;
    uiGenInput*		productfld_;
    uiGenInput*		creatorfld_;
    uiGenInput*		filenmfld_;
    uiGenInput*		versionfld_;
    uiTextEdit*		infofld_;
    uiTextEdit*		licensefld_;

    bool		rejectOK(CallBacker*);
    void		fillList();
    void		emptyFields();
    void		selChg(CallBacker*);
    void		loadPush(CallBacker*);

};


#endif
