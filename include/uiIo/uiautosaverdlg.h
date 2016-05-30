#ifndef uiautosaverdlg_h
#define uiautosaverdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2016
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
class uiGenInput;


mExpClass(uiIo) uiAutoSaverDlg : public uiDialog
{ mODTextTranslationClass(uiConvertPos);

public:
                        uiAutoSaverDlg(uiParent*);

private:

    uiGenInput*		isactivefld_;
    uiGenInput*		usehiddenfld_;
    uiGenInput*		nrsecondsfld_;

    void		isActiveCB(CallBacker*);
    bool		acceptOK(CallBacker*);

};

#endif
